/*
    For web frontend development purposes only
    Mocks the minimum functionality from *WebServer.cpp* for the frontend to run separate from Embedded stack

    Must be kept as simple as possible. No extra abstractions or complex business logic.
*/
const express = require('express');
const path =  require('path');

const app = express();
const dataDir = path.join(__dirname, '../data');

// mock objects
const config =  {
    displaysettings: {
        disp0: "",
        disp1: "",
        disp2: "",
        dispOff: ""
    }
};

const network =  {
    networksettings: {
        ssid: ""
    }
};

const logs = {
    rawlog: {
        enabled: true,
        filesize: 1
    }
};

// web document paths
app.get('/', (req, res) => {
    res.sendFile(dataDir + '/html/index.html');
});

app.get('/config', (req, res) => {
    res.sendFile(dataDir + '/html/config.html');
});

app.get('/network', (req, res) => {
    res.sendFile(dataDir + '/html/network.html');
});

app.get('/logs', (req, res) => {
    res.sendFile(dataDir + '/html/logs.html');
});

app.get('/debug', (req, res) => {
    res.sendFile(dataDir + '/html/debug.html');
});

// web service endpoints
app.get('/config_update', (req, res) => {
    res.json(config);
});

app.post('/config_save', (req, res) => {
    res.redirect('/config');
});

app.get('/network_update', (req, res) => {
    res.json(network);
});

app.post('/network_save', (req, res) => {
    res.redirect('/network');
});

app.get('/debug_update', (req, res) => {
    res.json({});
});

app.post('/debug_save', (req, res) => {
    res.redirect('/debug');
});

app.get('/logs_update', (req, res) => {
    res.json(logs);
});

app.get('/log_download', (req, res) => {
    res.sendStatus(404);
});

app.get('/log_delete', (req, res) => {
    res.redirect('/logs');
});

app.post('/logs_save', (req, res) => {
    res.redirect('/logs');
});

// fallback other assets like CSS and JS
app.use(express.static('../data'));

app.listen(8080, () => console.log('Started CANserver UI development server on port 8080'));