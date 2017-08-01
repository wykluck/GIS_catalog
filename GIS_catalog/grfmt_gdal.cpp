/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

// GDAL Macros
//#include "cvconfig.h"


// Our Header
#include "grfmt_gdal.hpp"
#include <ogr_spatialref.h>

/// C++ Standard Libraries
#include <iostream>
#include <stdexcept>
#include <string>
#include <opencv2/opencv.hpp>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
using namespace cv;
namespace cvGIS{


/**
 * Convert GDAL Palette Interpretation to OpenCV Pixel Type
*/
int  gdalPaletteInterpretation2OpenCV( GDALPaletteInterp const& paletteInterp, GDALDataType const& gdalType ){

    switch( paletteInterp ){

        /// GRAYSCALE
        case GPI_Gray:
            if( gdalType == GDT_Byte    ){ return CV_8UC1;  }
            if( gdalType == GDT_UInt16  ){ return CV_16UC1; }
            if( gdalType == GDT_Int16   ){ return CV_16SC1; }
            if( gdalType == GDT_UInt32  ){ return CV_32SC1; }
            if( gdalType == GDT_Int32   ){ return CV_32SC1; }
            if( gdalType == GDT_Float32 ){ return CV_32FC1; }
            if( gdalType == GDT_Float64 ){ return CV_64FC1; }
            return -1;

        /// RGB
        case GPI_RGB:
            if( gdalType == GDT_Byte    ){ return CV_8UC1;  }
            if( gdalType == GDT_UInt16  ){ return CV_16UC3; }
            if( gdalType == GDT_Int16   ){ return CV_16SC3; }
            if( gdalType == GDT_UInt32  ){ return CV_32SC3; }
            if( gdalType == GDT_Int32   ){ return CV_32SC3; }
            if( gdalType == GDT_Float32 ){ return CV_32FC3; }
            if( gdalType == GDT_Float64 ){ return CV_64FC3; }
            return -1;


        /// otherwise
        default:
            return -1;

    }
}

/**
 * Convert gdal type to opencv type
*/
int gdal2opencv( const GDALDataType& gdalType, const int& channels ){

    switch( gdalType ){

        /// UInt8
        case GDT_Byte:
            if( channels == 1 ){ return CV_8UC1; }
            if( channels == 3 ){ return CV_8UC3; }
            if( channels == 4 ){ return CV_8UC4; }
            else { return CV_8UC(channels); }
            return -1;

        /// UInt16
        case GDT_UInt16:
            if( channels == 1 ){ return CV_16UC1; }
            if( channels == 3 ){ return CV_16UC3; }
            if( channels == 4 ){ return CV_16UC4; }
            else { return CV_16UC(channels); }
            return -1;

        /// Int16
        case GDT_Int16:
            if( channels == 1 ){ return CV_16SC1; }
            if( channels == 3 ){ return CV_16SC3; }
            if( channels == 4 ){ return CV_16SC4; }
            else { return CV_16SC(channels); }
            return -1;

        /// UInt32
        case GDT_UInt32:
        case GDT_Int32:
            if( channels == 1 ){ return CV_32SC1; }
            if( channels == 3 ){ return CV_32SC3; }
            if( channels == 4 ){ return CV_32SC4; }
            else { return CV_32SC(channels); }
            return -1;

        case GDT_Float32:
            if( channels == 1 ){ return CV_32FC1; }
            if( channels == 3 ){ return CV_32FC3; }
            if( channels == 4 ){ return CV_32FC4; }
            else { return CV_32FC(channels); }
            return -1;

        case GDT_Float64:
            if( channels == 1 ){ return CV_64FC1; }
            if( channels == 3 ){ return CV_64FC3; }
            if( channels == 4 ){ return CV_64FC4; }
            else { return CV_64FC(channels); }
            return -1;

        default:
            std::cout << "Unknown GDAL Data Type" << std::endl;
            std::cout << "Type: " << GDALGetDataTypeName(gdalType) << std::endl;
            return -1;
    }

    return -1;
}

/**
 * GDAL Decoder Constructor
*/
GdalDecoder::GdalDecoder() :
	m_xBlockSize(-1), m_yBlockSize(-1),
	m_xBlocks(-1), m_yBlocks(-1),
	m_supportBlockRead(false)
{


    // set a dummy signature
    m_signature="0";
    for( size_t i=0; i<160; i++ ){
        m_signature += "0";
    }


    /// Register the driver
    GDALAllRegister();

    m_driver = NULL;
    m_dataset = NULL;
}

/**
 * GDAL Decoder Destructor
*/
GdalDecoder::~GdalDecoder(){


    if( m_dataset != NULL ){
       close();
    }
}

/**
 * Convert data range
*/
double range_cast( const GDALDataType& gdalType,
                   const int& cvDepth,
                   const double& value )
{

    // uint8 -> uint8
    if( gdalType == GDT_Byte && cvDepth == CV_8U ){
        return value;
    }
    // uint8 -> uint16
    if( gdalType == GDT_Byte && (cvDepth == CV_16U || cvDepth == CV_16S)){
        return (value*256);
    }

    // uint8 -> uint32
    if( gdalType == GDT_Byte && (cvDepth == CV_32F || cvDepth == CV_32S)){
        return (value*16777216);
    }

    // int16 -> uint8
    if( (gdalType == GDT_UInt16 || gdalType == GDT_Int16) && cvDepth == CV_8U ){
        return std::floor(value/256.0);
    }

    // int16 -> int16
    if( (gdalType == GDT_UInt16 || gdalType == GDT_Int16) &&
        ( cvDepth == CV_16U     ||  cvDepth == CV_16S   )){
        return value;
    }

    // float32 -> float32
    // float64 -> float64
    if( (gdalType == GDT_Float32 || gdalType == GDT_Float64) &&
        ( cvDepth == CV_32F     ||  cvDepth == CV_64F   )){
        return value;
    }

    std::cout << GDALGetDataTypeName( gdalType ) << std::endl;
    std::cout << "warning: unknown range cast requested." << std::endl;
    return (value);
}


/**
 * There are some better mpl techniques for doing this.
*/
void write_pixel( const double& pixelValue,
                  const GDALDataType& gdalType,
                  const int& gdalChannels,
                  Mat& image,
                  const int& row,
                  const int& col,
                  const int& channel ){

    // convert the pixel
    double newValue = range_cast(gdalType, image.depth(), pixelValue );

    // input: 1 channel, output: 1 channel
    if( gdalChannels == 1 && image.channels() == 1 ){
        if( image.depth() == CV_8U ){       image.ptr<uchar>(row)[col]          = newValue; }
        else if( image.depth() == CV_16U ){ image.ptr<unsigned short>(row)[col] = newValue; }
        else if( image.depth() == CV_16S ){ image.ptr<short>(row)[col]          = newValue; }
        else if( image.depth() == CV_32S ){ image.ptr<int>(row)[col]            = newValue; }
        else if( image.depth() == CV_32F ){ image.ptr<float>(row)[col]          = newValue; }
        else if( image.depth() == CV_64F ){ image.ptr<double>(row)[col]         = newValue; }
        else{ throw std::runtime_error("Unknown image depth, gdal: 1, img: 1"); }
    }

    // input: 1 channel, output: 3 channel
    else if( gdalChannels == 1 && image.channels() == 3 ){
        if( image.depth() == CV_8U ){        image.ptr<Vec3b>(row)[col] = Vec3b(newValue,newValue,newValue); }
        else if( image.depth() == CV_16U ){  image.ptr<Vec3s>(row)[col] = Vec3s(newValue,newValue,newValue); }
        else if( image.depth() == CV_16S ){  image.ptr<Vec3s>(row)[col] = Vec3s(newValue,newValue,newValue); }
        else if( image.depth() == CV_32S ){  image.ptr<Vec3i>(row)[col] = Vec3i(newValue,newValue,newValue); }
        else if( image.depth() == CV_32F ){  image.ptr<Vec3f>(row)[col] = Vec3f(newValue,newValue,newValue); }
        else if( image.depth() == CV_64F ){  image.ptr<Vec3d>(row)[col] = Vec3d(newValue,newValue,newValue); }
        else{                          throw std::runtime_error("Unknown image depth, gdal:1, img: 3"); }
    }

    // input: 3 channel, output: 1 channel
    else if( gdalChannels == 3 && image.channels() == 1 ){
        if( image.depth() == CV_8U ){   image.ptr<uchar>(row)[col] += (newValue/3.0); }
        else{ throw std::runtime_error("Unknown image depth, gdal:3, img: 1"); }
    }

    // input: 4 channel, output: 1 channel
    else if( gdalChannels == 4 && image.channels() == 1 ){
        if( image.depth() == CV_8U ){   image.ptr<uchar>(row)[col] = newValue;  }
        else{ throw std::runtime_error("Unknown image depth, gdal: 4, image: 1"); }
    }

    // input: 3 channel, output: 3 channel
    else if( gdalChannels == 3 && image.channels() == 3 ){
        if( image.depth() == CV_8U ){  image.at<Vec3b>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_16U ){  image.ptr<Vec3s>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_16S ){  image.ptr<Vec3s>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_32S ){  image.ptr<Vec3i>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_32F ){  image.ptr<Vec3f>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_64F ){  image.ptr<Vec3d>(row,col)[channel] = newValue;  }
        else{ throw std::runtime_error("Unknown image depth, gdal: 3, image: 3"); }
    }

    // input: 4 channel, output: 3 channel
    else if( gdalChannels == 4 && image.channels() == 3 ){
        if( channel >= 4 ){ return; }
        else if( image.depth() == CV_8U  && channel < 4 ){  image.ptr<Vec3b>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_16U && channel < 4 ){  image.ptr<Vec3s>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_16S && channel < 4 ){  image.ptr<Vec3s>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_32S && channel < 4 ){  image.ptr<Vec3i>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_32F && channel < 4 ){  image.ptr<Vec3f>(row,col)[channel] = newValue;  }
        else if( image.depth() == CV_64F && channel < 4 ){  image.ptr<Vec3d>(row,col)[channel] = newValue;  }
        else{ throw std::runtime_error("Unknown image depth, gdal: 4, image: 3"); }
    }

    // input: 4 channel, output: 4 channel
    else if( gdalChannels == 4 && image.channels() == 4 ){
        if( image.depth() == CV_8U ){  image.at<Vec4b>(row,col)[channel] = newValue;  }
        else{ throw std::runtime_error("Unknown image depth, gdal: 4, image: 4"); }
    }

    // input: > 4 channels, output: > 4 channels
    else if( gdalChannels > 4 && image.channels() > 4 ){
        if( image.depth() == CV_8U ){       image.ptr<uchar>(row,col)[channel]          = newValue; }
        else if( image.depth() == CV_16U ){ image.ptr<unsigned short>(row,col)[channel] = newValue; }
        else if( image.depth() == CV_16S ){ image.ptr<short>(row,col)[channel]          = newValue; }
        else if( image.depth() == CV_32S ){ image.ptr<int>(row,col)[channel]            = newValue; }
        else if( image.depth() == CV_32F ){ image.ptr<float>(row,col)[channel]          = newValue; }
        else if( image.depth() == CV_64F ){ image.ptr<double>(row,col)[channel]         = newValue; }
        else{ throw std::runtime_error("Unknown image depth, gdal: N, img: N"); }
    }
    // otherwise, throw an error
    else{
        throw std::runtime_error("error: can't convert types.");
    }

}


void write_ctable_pixel( const double& pixelValue,
                         const GDALDataType& gdalType,
                         GDALColorTable const* gdalColorTable,
                         Mat& image,
                         const int& y,
                         const int& x,
                         const int& c ){

    if( gdalColorTable == NULL ){
       write_pixel( pixelValue, gdalType, 1, image, y, x, c );
    }

    // if we are Grayscale, then do a straight conversion
    if( gdalColorTable->GetPaletteInterpretation() == GPI_Gray ){
        write_pixel( pixelValue, gdalType, 1, image, y, x, c );
    }

    // if we are rgb, then convert here
    else if( gdalColorTable->GetPaletteInterpretation() == GPI_RGB ){

        // get the pixel
        short r = gdalColorTable->GetColorEntry( (int)pixelValue )->c1;
        short g = gdalColorTable->GetColorEntry( (int)pixelValue )->c2;
        short b = gdalColorTable->GetColorEntry( (int)pixelValue )->c3;
        short a = gdalColorTable->GetColorEntry( (int)pixelValue )->c4;

        write_pixel( r, gdalType, 4, image, y, x, 2 );
        write_pixel( g, gdalType, 4, image, y, x, 1 );
        write_pixel( b, gdalType, 4, image, y, x, 0 );
        if( image.channels() > 3 ){
            write_pixel( a, gdalType, 4, image, y, x, 1 );
        }
    }

    // otherwise, set zeros
    else{
        write_pixel( pixelValue, gdalType, 1, image, y, x, c );
    }
}

bool GdalDecoder::readBlockData(int xBlockIndex, int yBlockIndex, Mat& blockImg)
{
	if (!m_supportBlockRead)
		return false;

	// make sure the raster is alive
	if (m_dataset == NULL || m_driver == NULL) {
		return false;
	}

	//TODO: assume the datatype are the same across different bands
	const GDALDataType gdalType = m_dataset->GetRasterBand(1)->GetRasterDataType();
	Mat readBuffer;
	int cvImgeType, readBufferType;
	switch (gdalType)
	{
	case GDT_Byte:
		cvImgeType = CV_MAKETYPE(CV_8U, m_dataset->GetRasterCount());
		readBufferType = CV_8U;
		break;
	case GDT_UInt16:
		cvImgeType = CV_MAKETYPE(CV_16U, m_dataset->GetRasterCount());
		readBufferType = CV_16U;
		break;
	case GDT_Int16:
		cvImgeType = CV_MAKETYPE(CV_16S, m_dataset->GetRasterCount());
		readBufferType = CV_16S;
		break;
	case GDT_Int32:
		cvImgeType = CV_MAKETYPE(CV_32S, m_dataset->GetRasterCount());
		readBufferType = CV_32S;
		break;
	case GDT_Float32:
		cvImgeType = CV_MAKETYPE(CV_32F, m_dataset->GetRasterCount());
		readBufferType = CV_32F;
		break;
	case GDT_Float64:
		cvImgeType = CV_MAKETYPE(CV_64F, m_dataset->GetRasterCount());
		readBufferType = CV_64F;
		break;
	default:
		//unsupported type conversion, return false
		return false;
	}

	blockImg.create(m_yBlockSize, m_xBlockSize, cvImgeType);
	readBuffer.create(m_yBlockSize, m_xBlockSize, readBufferType);

	// set the image to zero
	blockImg = 0;


	// iterate over each raster band
	// note that OpenCV does bgr rather than rgb
	int nChannels = m_dataset->GetRasterCount();

	//TODO: need to handle color table case
	GDALColorTable* gdalColorTable = NULL;
	if (m_dataset->GetRasterBand(1)->GetColorTable() != NULL) {
		gdalColorTable = m_dataset->GetRasterBand(1)->GetColorTable();
	}

	int xValid, yValid;
	// compute the portion of the block that is valid
	// for partial edge blocks.
	if ((xBlockIndex + 1) * m_xBlockSize > m_dataset->GetRasterXSize())
		xValid = m_dataset->GetRasterXSize() - xBlockIndex * m_xBlockSize;
	else
		xValid = m_xBlockSize;

	if ((yBlockIndex + 1) * m_yBlockSize > m_dataset->GetRasterYSize())
		yValid = m_dataset->GetRasterYSize() - yBlockIndex * m_yBlockSize;
	else
		yValid = m_yBlockSize;

	//need to read as BGR order
	for (int c = 0; c<nChannels; c++) {

		// get the GDAL Band
		GDALRasterBand* band = m_dataset->GetRasterBand(c + 1);
		//OpenCV requires the order has to be 'BGR' when reading into a cv::Mat
		auto targetChannel = c;
		switch (band->GetColorInterpretation())
		{
		case GCI_RedBand:
			targetChannel = 2;
			break;
		case GCI_GreenBand:
			targetChannel = 1;
			break;
		case GCI_BlueBand:
			targetChannel = 0;
			break;
		}

		// set the read buffer to zero
		readBuffer = 0;


		// get the entire block
		band->ReadBlock(xBlockIndex, yBlockIndex, readBuffer.data);

		// iterate over each row and column
		for (int y = 0; y < yValid; y++) {


			// set inside the image
			for (int x = 0; x< xValid; x++) {

				//TODO: could use Intel IPP to convert 
				switch (gdalType)
				{
				case GDT_Byte:
					blockImg.ptr<uchar>(y, x)[targetChannel]
						= readBuffer.at<uchar>(y, x);
					break;

				case GDT_UInt16:
					blockImg.ptr<ushort>(y, x)[targetChannel]
						= readBuffer.at<ushort>(y, x);
					break;

				case GDT_Int16:
					blockImg.ptr<short>(y, x)[targetChannel]
						= readBuffer.at<short>(y, x);
					break;

				case GDT_Int32:
					blockImg.ptr<int>(y, x)[targetChannel]
						= readBuffer.at<int>(y, x);
					break;

				case GDT_Float32:
					blockImg.ptr<float>(y, x)[targetChannel]
						= readBuffer.at<float>(y, x);
					break;

				case GDT_Float64:
					blockImg.ptr<double>(y, x)[targetChannel]
						= readBuffer.at<double>(y, x);
					break;

				default:
					//unsupported type conversion, return false
					return false;
				}

				// set depending on image types
				//   given boost, I would use enable_if to speed up.  Avoid for now.
				/*if (hasColorTable == false) {
					write_pixel(buffer[x], gdalType, nChannels, blockImg, y, x, c);
				}
				else {
					write_ctable_pixel(buffer[x], gdalType, gdalColorTable, blockImg, y, x, c);
				}
				*/
			}
		}


	}

	return true;

}

bool GdalDecoder::readThumbnailData(cv::Mat& mat)
{
	// iterate over each raster band
	// note that OpenCV does bgr rather than rgb
	int nChannels = m_dataset->GetRasterCount();

	GDALColorTable* gdalColorTable = NULL;
	if (m_dataset->GetRasterBand(1)->GetColorTable() != NULL) {
		gdalColorTable = m_dataset->GetRasterBand(1)->GetColorTable();
	}

	const GDALDataType gdalType = m_dataset->GetRasterBand(1)->GetRasterDataType();
	int nRows, nCols;

	if (nChannels > mat.channels()) {
		nChannels = mat.channels();
	}

	// create a temporary scanline pointer to store data
	double* uBuffer = new double[mat.cols * mat.rows];
	for (int c = 0; c<nChannels; c++) {

		// get the GDAL Band
		GDALRasterBand* band = m_dataset->GetRasterBand(c + 1);

		// make sure the image band has the same dimensions as the image
		if (band->GetXSize() != m_width || band->GetYSize() != m_height) { return false; }

		// grab the raster size
		nRows = band->GetYSize();
		nCols = band->GetXSize();

		//OpenCV requires the order has to be 'BGR' when reading into a cv::Mat
		auto targetChannel = c;
		switch (band->GetColorInterpretation())
		{
		case GCI_RedBand:
			targetChannel = 2;
			break;
		case GCI_GreenBand:
			targetChannel = 1;
			break;
		case GCI_BlueBand:
			targetChannel = 0;
			break;
		}

		band->RasterIO(GF_Read, 0, 0, nCols, nRows, uBuffer, mat.cols, mat.rows,
			GDT_Float64, 0, 0);

		// iterate over each row and column
		for (int y = 0; y<mat.rows; y++) {

			// set inside the image
			for (int x = 0; x<mat.cols; x++) {

				// set depending on image types
				//   given boost, I would use enable_if to speed up.  Avoid for now.
				if (hasColorTable == false) {
					write_pixel(uBuffer[y * mat.cols + x], gdalType, nChannels, mat, y, x, targetChannel);
				}
				else {
					write_ctable_pixel(uBuffer[y * mat.cols + x], gdalType, gdalColorTable, mat, y, x, targetChannel);
				}
			}
		}
	}
	// delete our temp pointer
	delete[] uBuffer;
	return true;
}


bool GdalDecoder::generateThumbnail(int width, int height, std::vector<uchar>& thumbnailBuffer)
{
	cv::Mat thumbnailMat(width, height, CV_8UC3);
	readThumbnailData(thumbnailMat);
	//std::string thumbnailPath = "c:\\thumbnails\\";
	//thumbnailPath.append(fs::path(m_filename.c_str()).stem().string()).append(".png");
	//return imwrite(thumbnailPath, thumbnailMat);
	return imencode(".png", thumbnailMat, thumbnailBuffer);
}


/**
 * read data
*/
bool GdalDecoder::readData( Mat& img ){


    // make sure the image is the proper size
    if( img.size().height != m_height ){
        return false;
    }
    if( img.size().width != m_width ){
        return false;
    }

    // make sure the raster is alive
    if( m_dataset == NULL || m_driver == NULL ){
        return false;
    }

    // set the image to zero
    img = 0;


    // iterate over each raster band
    // note that OpenCV does bgr rather than rgb
    int nChannels = m_dataset->GetRasterCount();

    GDALColorTable* gdalColorTable = NULL;
    if( m_dataset->GetRasterBand(1)->GetColorTable() != NULL ){
        gdalColorTable = m_dataset->GetRasterBand(1)->GetColorTable();
    }

    const GDALDataType gdalType = m_dataset->GetRasterBand(1)->GetRasterDataType();
    int nRows, nCols;

    if( nChannels > img.channels() ){
        nChannels = img.channels();
    }

    for( int c = 0; c<nChannels; c++ ){

        // get the GDAL Band
        GDALRasterBand* band = m_dataset->GetRasterBand(c+1);

        // make sure the image band has the same dimensions as the image
        if( band->GetXSize() != m_width || band->GetYSize() != m_height ){ return false; }

        // grab the raster size
        nRows = band->GetYSize();
        nCols = band->GetXSize();

        // create a temporary scanline pointer to store data
        double* scanline = new double[nCols];

        // iterate over each row and column
        for( int y=0; y<nRows; y++ ){

            // get the entire row
            band->RasterIO( GF_Read, 0, y, nCols, 1, scanline, nCols, 1, GDT_Float64, 0, 0);

            // set inside the image
            for( int x=0; x<nCols; x++ ){

                // set depending on image types
                //   given boost, I would use enable_if to speed up.  Avoid for now.
                if( hasColorTable == false ){
                    write_pixel( scanline[x], gdalType, nChannels, img, y, x, c );
                }
                else{
                    write_ctable_pixel( scanline[x], gdalType, gdalColorTable, img, y, x, c );
                }
            }
        }

        // delete our temp pointer
        delete [] scanline;


    }

    return true;
}


/**
 * Read image header
*/
bool GdalDecoder::readHeader(){

    // load the dataset
    m_dataset = (GDALDataset*) GDALOpen( m_filename.c_str(), GA_ReadOnly);

    // if dataset is null, then there was a problem
    if( m_dataset == NULL ){
        return false;
    }

    // make sure we have pixel data inside the raster
    if( m_dataset->GetRasterCount() <= 0 ){
        return false;
    }

    //extract the driver infomation
    m_driver = m_dataset->GetDriver();

    // if the driver failed, then exit
    if( m_driver == NULL ){
        return false;
    }


    // get the image dimensions
    m_width = m_dataset->GetRasterXSize();
    m_height= m_dataset->GetRasterYSize();

    // make sure we have at least one band/channel
    if( m_dataset->GetRasterCount() <= 0 ){
        return false;
    }

    // check if we have a color palette
    int tempType;
    if( m_dataset->GetRasterBand(1)->GetColorInterpretation() == GCI_PaletteIndex ){

        // remember that we have a color palette
        hasColorTable = true;

        // if the color tables does not exist, then we failed
        if( m_dataset->GetRasterBand(1)->GetColorTable() == NULL ){
            return false;
        }

        // otherwise, get the pixeltype
        else{
            // convert the palette interpretation to opencv type
            tempType = gdalPaletteInterpretation2OpenCV( m_dataset->GetRasterBand(1)->GetColorTable()->GetPaletteInterpretation(),
                                                         m_dataset->GetRasterBand(1)->GetRasterDataType() );

            if( tempType == -1 ){
                return false;
            }
            m_type = tempType;
        }

    }

    // otherwise, we have standard channels
    else{

        // remember that we don't have a color table
        hasColorTable = false;

        // convert the datatype to opencv
        tempType = gdal2opencv( m_dataset->GetRasterBand(1)->GetRasterDataType(), m_dataset->GetRasterCount() );
        if( tempType == -1 ){
            return false;
        }
        m_type = tempType;
    }

	//get block reading information
	m_dataset->GetRasterBand(1)->GetBlockSize(&m_xBlockSize, &m_yBlockSize);
	for (auto bandIndex = 2; bandIndex <= m_dataset->GetRasterCount(); bandIndex++)
	{
		int xBlockSize, yBlockSize;
		m_dataset->GetRasterBand(bandIndex)->GetBlockSize(&xBlockSize, &yBlockSize);
		if (xBlockSize != m_xBlockSize || yBlockSize != m_yBlockSize)
		{
			m_supportBlockRead = false;
			break;
		}
	}
	m_supportBlockRead = true;

	m_xBlocks = (m_dataset->GetRasterXSize() + m_xBlockSize - 1) / m_xBlockSize;
	m_yBlocks = (m_dataset->GetRasterYSize() + m_yBlockSize - 1) / m_yBlockSize;

	//extract cell size
	double unitInMeters = 1.0;
	m_imageMetadata.width = m_width;
	m_imageMetadata.height = m_height;
	m_imageMetadata.bandCount = m_dataset->GetRasterCount();
	m_imageMetadata.datasetPath = m_filename;
	m_imageMetadata.dataTypeSizeInBits = GDALGetDataTypeSize(m_dataset->GetRasterBand(1)->GetRasterDataType());
	double geoTransform[6];
	if (m_dataset->GetGeoTransform(geoTransform) == CE_None)
	{
		for (auto i = 0; i < 6; i++)
			m_imageMetadata.geoTransformParams.push_back(geoTransform[i]);
		printf(" GeoTransform: %f, %f, %f, %f, %f, %f\n", geoTransform[0], geoTransform[1], geoTransform[2],
			geoTransform[3], geoTransform[4], geoTransform[5]);
	}
	OGRSpatialReference ogr(m_dataset->GetProjectionRef());

	auto authorityName = ogr.GetAuthorityName("PROJCS");
	if (authorityName != NULL)
	{
		auto authorityCode = ogr.GetAuthorityCode("PROJCS");
		m_imageMetadata.spatialId.append(authorityName).append(":")
			.append(authorityCode);
	}
	m_imageMetadata.updatedTime = std::time(nullptr);	

    return true;
}

/**
 * Close the module
*/
void GdalDecoder::close(){


    GDALClose((GDALDatasetH)m_dataset);
    m_dataset = NULL;
    m_driver = NULL;
}



/**
 * Test the file signature
*/
bool GdalDecoder::checkSignature( const String& signature )const{


    // look for NITF
    std::string str(signature);
    if( str.substr(0,4).find("NITF") != std::string::npos ){
        return true;
    }

    // look for DTED
    if( str.substr(140,4) == "DTED" ){
        return true;
    }

    return false;
}

} /// End of cv Namespace
