var datasetCrawler = require('./datasetCrawler');
var CronJob = require('cron').CronJob;
var fs = require('fs');
var gdal_binding = require('bindings')('addon.node');
var path = require('path');
var mkdirp = require('mkdirp');

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

readConfig.then((configObj) => {
    let logFilePath = null;
    if (configObj.hasOwnProperty("logFileDirectory") && configObj["logFileDirectory"] !== null) {
        logFilePath = configObj["logFileDirectory"];
    }
    else
        logFilePath = process.cwd() + path.sep + "logs";
    mkdirp.sync(logFilePath);
    logFilePath += path.sep + "UpdateDataset.log";
    gdal_binding.Init(logFilePath);
    var job = new CronJob({
        cronTime: configObj.scanCronPeriod,
        onTick: function () {
            console.log("Started to update all dataset information.\n");
            datasetCrawler.updateAllDatasetInfo(configObj);
            console.log("Finished updating all dataset information.\n");
        },
        start: false,
        timeZone: 'America/Los_Angeles'
    });
    job.start();
}).catch((err) => {
    console.log(err);
});;




