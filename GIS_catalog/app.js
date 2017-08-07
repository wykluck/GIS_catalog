var datasetCrawler = require('./datasetCrawler');
var CronJob = require('cron').CronJob;
var fs = require('fs');

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




