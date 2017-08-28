#include "grfmt_gdal.hpp"
#include <opencv2/opencv.hpp>
using namespace cv;
namespace cvGIS {

	bool GdalDecoder::readThumbnailData(cv::Mat& mat)
	{
		//band selection first
		std::map<int, int> bandMapping;
		bool isRGBModel = false;
		const GDALDataType gdalType = m_dataset->GetRasterBand(1)->GetRasterDataType();
		// note that OpenCV use bgr as band order rather than rgb
		for (int c = 1; c <= m_imageMetadata.bandCount; c++) {
			switch (m_dataset->GetRasterBand(c)->GetColorInterpretation())
			{
			case GCI_RedBand:
				bandMapping[c] = 2;
				isRGBModel = true;
				break;
			case GCI_GreenBand:
				bandMapping[c] = 1;
				isRGBModel = true;
				break;
			case GCI_BlueBand:
				bandMapping[c] = 0;
				isRGBModel = true;
				break;
			case GCI_GrayIndex:
				bandMapping[c] = c;
				break;
			}
		}
		if (isRGBModel)
		{
			if (bandMapping.size() < 3)//not a complete rgb model
				return false;
		}
		else
		{
			//not a rgb model, just use band 1 or (1, 2, 3)
			for (int c = 1; c <= m_imageMetadata.bandCount; c++) {
				bandMapping[c] = c - 1;
				if (bandMapping.size() == 3)
					break;
			}
			if (bandMapping.size() == 2) //can only be 1 channel or 3 channels
				return false;
		}


		std::vector<cv::Mat> bufferMatVec;

		//copy band data into opencv buffer
		for (auto bandMapPair : bandMapping) {

			// get the GDAL Band
			GDALRasterBand* band = m_dataset->GetRasterBand(bandMapPair.first);

			// make sure the image band has the same dimensions as the image
			if (band->GetXSize() != m_width || band->GetYSize() != m_height) { return false; }

			if (gdalType == GDT_Byte)
			{
				cv::Mat ucharMat(mat.rows, mat.cols, CV_8UC1);
				band->RasterIO(GF_Read, 0, 0, band->GetXSize(), band->GetYSize(), ucharMat.data, mat.cols, mat.rows,
					gdalType, 0, 0);
				bufferMatVec.push_back(ucharMat);
			}
			else
			{
				cv::Mat ushortMat(mat.rows, mat.cols, CV_16UC1);
				band->RasterIO(GF_Read, 0, 0, band->GetXSize(), band->GetYSize(), ushortMat.data, mat.cols, mat.rows,
					gdalType, 0, 0);
				bufferMatVec.push_back(ushortMat);
			}

		}
		if (!bufferMatVec.empty())
		{
			if (gdalType != GDT_Byte)
			{
				std::vector<cv::Mat> resMatVec(bufferMatVec.size());
				std::size_t i = 0;
				for (auto bufferMat : bufferMatVec)
				{
					double min, max;
					cv::minMaxIdx(bufferMat, &min, &max);
					if (max != min)
						bufferMat.convertTo(resMatVec[i], CV_8UC1, 256.0 / (max - min + 1), -min / (max - min + 1));
					else
						bufferMat.convertTo(resMatVec[i], CV_8UC1, 255.0 / max);
					i++;
				}
				cv::merge(&resMatVec[0], resMatVec.size(), mat);
			}
			else
				cv::merge(&bufferMatVec[0], bufferMatVec.size(), mat);
		}

		return true;
	}


	bool GdalDecoder::generateThumbnail(int width, int height, std::vector<uchar>& thumbnailBuffer)
	{
		cv::Mat *pThumbnailMat = nullptr;
		const GDALDataType gdalType = m_dataset->GetRasterBand(1)->GetRasterDataType();
		if (gdalType != GDT_Byte && gdalType != GDT_UInt16)
			return false;
		if (m_imageMetadata.bandCount == 1)
		{
			pThumbnailMat = new cv::Mat(height, width, CV_8UC1);
		}
		else
		{
			pThumbnailMat = new cv::Mat(height, height, CV_8UC3);
		}
		bool res = false;
		if (readThumbnailData(*pThumbnailMat))
		{
			auto res = imencode(".jpg", *pThumbnailMat, thumbnailBuffer);
		}
		delete pThumbnailMat;
		return res;
	}



}