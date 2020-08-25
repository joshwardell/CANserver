/*
    For web frontend development purposes only
    Mocks the minimum functionality from *WebServer.cpp* for the frontend to run separate from Embedded stack

    Must be kept as simple as possible. No extra abstractions or complex business logic.
*/
const express = require('express');
const path =  require('path');

const app = express();
const dataDir = path.join(__dirname, '../ui-data');

// mock objects

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
    },
    "123a": {
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
    dynamicanalysisitems:{
        BattAmps:0,
        BattCoolantRate:0,
        BattVolts:0,
        DisplayOn:0,
        DistanceUnitMiles:0,
        MinBattTemp:0,
        RearTorque:0,
        VehSpeed:0
    },
    processeditems:{
        BSL:0,
        BSR:0,
        BattPower:0,
        BattPower_Scaled_Bar:0,
        RearTorque_Scaled_Bar:0,
        VehSpeed:0
    }
};

// web document paths
app.get('/', (req, res) => {
    res.sendFile(dataDir + '/html/index.html');
});

app.get('/displays', (req, res) => {
    res.sendFile(dataDir + '/html/displays.html');
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

app.get('/processing', (req, res) => {
    res.sendFile(dataDir + '/html/processing.html');
});




// web service endpoints
app.get('/display_load', (req, res) => {
    res.send('return "65535c" .. math.floor(CANServer_getVar("BattPower") * 10) .. "vWK  Bu" .. math.floor(CANServer_getVar("BattPower_Scaled_Bar")) .. "b0m100r"');
});
app.get('/display_stats', (req, res) => {
    res.json({
        state:true,
        errorstring:"",
        mean:0.9,
        mode:1,
        max:1,
        min:0,
        stddev:0.3
    });
});
app.get('/display_save', (req, res) => {
    res.status(200).end()
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

app.get('/analysis_info', (req, res) => {
    const { item } = req.query;
    res.json(analysis_load_json[item]);
});


app.get('/processing_script', (req, res) => {
    res.send("\
--[[ Sort out the wattage based on V*A --]]\n\
local battPower = (CANServer_getVar(\"BattVolts\") * CANServer_getVar(\"BattAmps\") / 1000.0);\n\
CANServer_setVar(\"BattPower\", battPower);\n\
\n\
--[[ Scale the battery power for the bargraph --]]\n\
local scaledBattPower = math.min(math.max((24) * (battPower) / (300), -24), 24);\n\
CANServer_setVar(\"BattPower_Scaled_Bar\", math.floor(scaledBattPower + 0.5));\n\
"
    );
});

app.get('/processing_stats', (req, res) => {
    res.json({
        state:true,
        errorstring:"",
        mean:0.3,
        mode:0,
        max:2,
        min:0,
        stddev:0.640313
    });
});



// fallback other assets like CSS and JS
app.use(express.static('../ui-data'));

app.listen(8080, () => console.log('Started CANserver UI development server on port 8080'));