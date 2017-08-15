var express = require('express')
var app = express()
var MongoClient = require('mongodb').MongoClient, assert = require('assert');
var bodyParser = require('body-parser');
var ObjectID = require('mongodb').ObjectID;

app.use(bodyParser.json()); // for parsing application/json
app.use(bodyParser.urlencoded({ extended: true })); // for parsing application/x-www-form-urlencoded

//TODO: Not sure post with query parameter in request body is correct way to do it.
app.post('/datasets/metadata', function (req, res) {
    datasetCollection.find(req.body).toArray(function (err, docs) {
        for (let doc of docs) {
            //delete the thumbnail image and add thumbnail url instead
            delete doc.thumbnail;
            doc["thumbnailUrl"] = req.protocol + '://' + req.get('host') + "/datasets/thumbnail/" + doc._id;
        }
        res.contentType('application/json');
        res.send(docs);
    });
})

app.get('/datasets/thumbnail/:id', function (req, res) {
    let objectId = new ObjectID(req.params.id);
    datasetCollection.findOne({ _id: objectId }, function (err, doc) {
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

app.listen(8086);