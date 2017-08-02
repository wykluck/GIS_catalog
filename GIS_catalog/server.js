var express = require('express')
var app = express()
var MongoClient = require('mongodb').MongoClient, assert = require('assert');

app.get('/docs', function (req, res) {
    datasetCollection.find({}).toArray(function (err, docs) {
        res.writeHead(200, { 'Content-Type': 'image/jpeg' });
        res.end(new Buffer.from(docs[0].thumbnail));
        //res.send(docs[0].thumbnail);
    });
})

// Connection URL 
var url = 'mongodb://localhost:27017/CatalogDB';
var datasetCollection = null;
// Use connect method to connect to the Server 
MongoClient.connect(url, function (err, db) {
    assert.equal(null, err);
    console.log("Connected correctly to server");
    datasetCollection = db.collection('Dataset');

});

app.listen(8080);