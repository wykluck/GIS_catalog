var express = require('express')
var app = express()
var MongoClient = require('mongodb').MongoClient, assert = require('assert');
var bodyParser = require('body-parser');

app.use(bodyParser.json()); // for parsing application/json
app.use(bodyParser.urlencoded({ extended: true })); // for parsing application/x-www-form-urlencoded

//TODO: Not sure post with query parameter in request body is correct way to do it.
app.post('/datasets/metadata', function (req, res) {
    datasetCollection.find(req.body).toArray(function (err, docs) {
        for (let doc of docs) {
            delete doc.thumbnail;
        }
        res.contentType('application/json');
        res.send(docs);
    });
})

app.post('/datasets/thumbnails', function (req, res) {
    //TODO: need to think about how to return multiple images as response
    datasetCollection.findOne(req.body, function (err, doc) {
        res.contentType('png');
        res.end(doc.thumbnail.buffer, 'binary');
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