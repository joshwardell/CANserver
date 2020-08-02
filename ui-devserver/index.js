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
        disp0: "65535c${BattPower}vWK  Bu${BattPower_Scaled_Bar}b0m100r",
        disp1: "65535c${RearTorque}vMNu${RearTorque_Scaled_Bar}b0m100r",
        disp2: "$if BSR {{2v63488c6m100r}} $elseif BSL {{1v63488c6m100r}} $else {{65535c${VehSpeed}v${SpeedUnitString}u${BattPower_Scaled_Bar}b0m100r}}",
        disp3: "1m2s DISPLAY    3   t500r",
    }
};

const network =  {
    networksettings: {
        ssid: "externalssid"
    }
};

const network_stationlist_json = {
    stations: [
        {
            mac: "DE:AD:BE:EF:BA:AD",
            ip: "192.168.4.2"
        },
        {
            mac: "BA:AD:FE:ED:DE:AD",
            ip: "192.168.4.3"
        }
    ]
};

const logs = {
    rawlog:{
       enabled:false,
       hasfile:true,
       filesize:123456
    },
    intervallog:{
       enabled:true,
       hasfile:true,
       filesize:54432
    },
    seriallog:{
       enabled:false,
       hasfile:false,
       filesize:0
    },
    sddetails:{
       available:true,
       totalkbytes:7875,
       usedkbytes:2
    }
};

const analysis_load_json = {
    TestVolts: {
        frameid: 306,
        startBit: 0,
        bitLength: 16,
        factor: 0.01,
        signalOffset: 0,
        isSigned: false,
        byteOrder: true,
        builtIn: true
    },
    TestAmps: {
        frameid: 306,
        startBit: 16,
        bitLength: 16,
        factor: -0.1,
        signalOffset: 0,
        isSigned: true,
        byteOrder: true
    }
};

const analysis_update_json = {
    TestVolts: 350,
    TestAmps: 99,
};

const debug_update_json = {
    vehiclestatus: {
      BattVolts: 350,
      BattAmps: 99,
      RearTorque: 1210,
      FrontTorque: 0,
      MinBattTemp: 0,
      BattCoolantRate: 0,
      PTCoolantRate: 0,
      MaxRegen: 45,
      MaxDisChg: 0,
      VehSpeed: 69,
      SpeedUnit: 0,
      v12v261: 0,
      BattCoolantTemp: 0,
      PTCoolantTemp: 0,
      BattRemainKWh: 0,
      BattFullKWh: 420,
      InvHStemp376: 0,
      BSR: 0,
      BSL: 0,
      DisplayOn: 1
    },
    dynamicanalysisitems: {
      TestVolts: 350,
      TestAmps: 99
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

app.get('/analysis', (req, res) => {
    res.sendFile(dataDir + '/html/analysis.html');
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

app.get('/network_stationlist', (req, res) => {
    res.json(network_stationlist_json);
});

app.get('/debug_update', (req, res) => {
    res.json(debug_update_json);
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

app.get('/analysis_load', (req, res) => {
    res.json(analysis_load_json);
});

app.get('/analysis_update', (req, res) => {
    res.json(analysis_update_json);
});

// fallback other assets like CSS and JS
app.use(express.static('../data'));

app.listen(8080, () => console.log('Started CANserver UI development server on port 8080'));