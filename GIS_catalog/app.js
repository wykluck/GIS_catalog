var datasetCrawler = require('./datasetCrawler');
var CronJob = require('cron').CronJob;
/*
var job = new CronJob({
    cronTime: '* * * * * *',
    onTick: function () {
        datasetCrawler.updateAllDatasetInfo();
    },
    start: false,
    timeZone: 'America/Los_Angeles'
});
job.start();
*/
datasetCrawler.updateAllDatasetInfo();


