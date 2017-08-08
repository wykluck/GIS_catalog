var fs = require('fs');
var path = require('path');
var process = require('process');
var gdal_binding = require('bindings')('addon.node');
var exports = module.exports = {};
gdal_binding.GdalInit();


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

exports.updateAllDatasetInfo = (configObj) => {
    gdal_binding.BeginUpdate(configObj.forceUpdate);
    let formatExtensionSet = new Set(configObj.formatExtensions);
    configObj.catalogBaseDirs.forEach((baseDir) => {
        traverseDirectory(baseDir, formatExtensionSet, gdal_binding.UpdateDatasetInfo);
    });
    gdal_binding.EndUpdate();
}
