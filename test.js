const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log('Client connected');

    ws.on('message', (message) => {
        console.log(`Received message: ${message}`);
    });

    ws.on('error', (err) => {
        console.log(err)
    })

    ws.send('Hello from JavaScript!');
});

server.listen(9013, () => {
    console.log('Server started on http://localhost:9013');
});


server.on('error', (err) => {
    console.log(err)
})