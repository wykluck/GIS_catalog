var fs = require('fs');
var path = require('path');
var process = require('process');
var gdal_binding = require('bindings')('addon.node');
var exports = module.exports = {};

var readConfig = new Promise((resolve, reject) => {
    fs.readFile('./config.json', 'utf8', function (err, data) {
        if (err) reject(err);
        //strip the potential bom marker
        data = data.replace(/^\uFEFF/, '');
        try {
            resolve(JSON.parse(data));
        }
        catch (err) {
            reject(err);
        }
    });
});

var retrieveDatasetInfo = (filePath) => {
    gdal_binding.RetrieveDatasetInfo(filePath);
}

var traverseDirectory = (dirname, formatExtensionSet, callback) => {
    var directory = [];
    childPathArr = fs.readdirSync(dirname);
    childPathArr.forEach(function (file) {
        file = dirname + '\\' + file;
        stat = fs.statSync(file);
        if (stat) {
            directory.push(file);
            if (stat && stat.isDirectory()) {
                traverseDirectory(file, formatExtensionSet, callback);
            } else {
                let lowerExtName = path.extname(file).toLowerCase();
                if (stat.isFile() && formatExtensionSet.has(lowerExtName)) {
                    callback(file);
                }
            }
        };
    });
}

var printAllDatasetInfo = () => {
    gdal_binding.GdalInit();
    readConfig.then((configObj) => {
        let formatExtensionSet = new Set(configObj.formatExtensions);
        configObj.catalogBaseDirs.forEach((baseDir) => {
            traverseDirectory(baseDir, formatExtensionSet, retrieveDatasetInfo);
        });
        gdal_binding.FinishCrawl();
    }).catch((err) => {
        process.send('Unable to open gdal file at ' + filePath);
    });
}

exports.printAllDatasetInfo = printAllDatasetInfo;