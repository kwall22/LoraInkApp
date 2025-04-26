const express = require("express");
const cors = require('cors');
const session = require('express-session');
const WebSocket = require('ws');
const model = require('./model');
const path = require('path');

const port = 8080;
const app = express();

app.use(express.urlencoded({extended: false}));
app.use(cors());
app.use(express.static("public"));
app.use(session({
    secret: "aiseqaf23easd3421wer9079a0dsfq3lkashfaSF",
    saveUninitialized: true,
    resave: false
}));

function authorizeRequest(request, response, next){
    if(request.session && request.session.userId){
        model.User.findOne({ _id: request.session.userId }).then(function (user) {
            request.user = user;
            next();
        })
    }
    else {
        response.status(401).send("Not authorized");
    }
}

app.post("/statusMessages", authorizeRequest, function (request, response) {
    const userId = request.session.userId;
    const MAX_STATUS_MESSAGES = 6;
    
    model.StatusMessage.countDocuments({ user: userId }).then((count) => {
        if (count >= MAX_STATUS_MESSAGES) {
            console.log("deleting oldest");
            return model.StatusMessage.findOneAndDelete({ user: userId }, { sort: { LastUsed: 1 } });
        
        }
        return Promise.resolve();
    }).then(() => {
        console.log("adding new one");
        const newStatusMessage = new model.StatusMessage({
            Content: request.body.Content,
            LastUsed: request.body.LastUsed,
            user: userId
        });
        return newStatusMessage.save();
    }).then(() => {
        var userStatusNodeId = "";
        model.User.findOne({_id: userId}).then(function (user) {
            userStatusNodeId = user.statusNodeId;
            if (clients["esp32"]) {
                const formattedData = {
                    eInkId: userStatusNodeId,
                    messageType: "status",
                    message: request.body.Content
                };
                clients["esp32"].send(JSON.stringify(formattedData));
                console.log("message sent to client:", formattedData);
            } else {
                console.log("no client!!");
            }
            response.set("Access-Control-Allow-Origin", "*");
            response.status(201).send("created");
        });
    }).catch((error) => {
        if (error === "Limit reached") return;
        if (error.errors) {
            const errorMessages = {};
            for (const fieldName in error.errors) {
                errorMessages[fieldName] = error.errors[fieldName].message;
            }
            response.status(422).json(errorMessages);
        } else {
            console.error(error);
            response.status(500).send("Unknown error creating entry.");
        }
    });
});

app.get("/statusMessages", authorizeRequest, function (request, response) {
    var filter = { user: request.user._id};
    model.StatusMessage.find(filter).populate('user').then((statusMessages) => {
        response.set("Access-Control-Allow-Origin", "*");
        response.json(statusMessages);
    });
});



app.put("/statusMessages/:statusMessageId", authorizeRequest, function (request, response) {
    const userId = request.user._id;
    console.log("at server, update message with id: ", request.params.statusMessageId);
    model.StatusMessage.findOne({_id: request.params.statusMessageId}).then((statusMessage) => {
        if (statusMessage){
            model.StatusMessage.updateOne({_id: request.params.statusMessageId, user: request.user._id}, {
                Content: request.body.Content,
                LastUsed: request.body.LastUsed,
                user: request.user._id
            }).then(() => {
                response.set("Access-Control-Allow-Origin", "*");
                response.status(200).send("updated");
            }).catch((error) => {
                if (error.errors){
                    var errorMessages = {};
                    for (var fieldName in error.errors){
                        errorMessages[fieldName] = error.errors[fieldName].message
                    }
                    console.error("failed to update status with ID: ", request.params.statusMessageId);
                    response.sendStatus(404).json(errorMessages);
                } else {
                    response.status(500).send("Unknown error: ", error);
                } 
            });
        } else {
            console.log("message with that id not found");
            response.sendStatus(404);
        }
    }).catch((error) => {
        console.error("failed to query status with ID: ", request.params.statusMessageId);
        response.sendStatus(404);
    });
    var userStatusNodeId = "";
    model.User.findOne({_id: userId}).then(function (user) {
        userStatusNodeId = user.statusNodeId;
        console.log( "in model", userStatusNodeId);
        if (clients["esp32"]) {
            console.log("about to send: ", userStatusNodeId);
            const formattedData = {
                eInkId: userStatusNodeId,
                messageType: "status",
                message: request.body.Content
            };
            clients["esp32"].send(JSON.stringify(formattedData));
            console.log("message sent to client:", formattedData);
        } else {
            console.log("No client!!");
        }
    });
});

app.delete("/statusMessages/:statusMessageId", authorizeRequest, function(request, response) {
    console.log("at server, delete message with ID: ", request.params.statusMessageId);
    model.StatusMessage.deleteOne({_id: request.params.statusMessageId, user: request.user._id}).then(() => {
        response.set("Access-Control-Allow-Origin", "*");
        response.status(200).send("deleted");
    }).catch((error) => {
        console.error("failed to delete status message with id: ", request.params.statusMessageId);
        response.sendStatus(404);
    });
});

app.post("/scheduleEvents", authorizeRequest, function(request, response) {
    const userId = request.session.userId;
    const newScheduleEvent = new model.ScheduleEvent({
        DayOfTheWeek: request.body.DayOfTheWeek,
        StartTime: request.body.StartTime,
        EndTime: request.body.EndTime,
        Description: request.body.Description,
        Color: request.body.Color,
        user: request.session.userId
    });
    newScheduleEvent.save().then(() => {
        response.set("Access-Control-Allow-Origin", "*");
        response.status(201).send("created");
    }).catch((error) => {
        if (error.errors){
            var errorMessages = {};
            for (var fieldName in error.errors){
                errorMessages[fieldName] = error.errors[fieldName].message;
            }
            response.status(422).json(errorMessages);
        } else{
            response.status(500).send("Unknown error creating entry.");
        }
    });
});

app.get("/scheduleEvents", authorizeRequest, function (request, response) {
    var filter = { user: request.user._id};
    model.ScheduleEvent.find(filter).populate('user').then((scheduleEvents) => {
        console.log("events from db: ", scheduleEvents);
        response.set("Access-Control-Allow-Origin", "*");
        response.json(scheduleEvents);
    });
});

app.put("/scheduleEvents/:scheduleEventId", authorizeRequest, function (request, response) {
    console.log("at server, update event with id: ", request.params.scheduleEventId);
    model.ScheduleEvent.findOne({_id: request.params.scheduleEventId}).then((scheduleEvent) => {
        if (scheduleEvent){
            model.ScheduleEvent.updateOne({_id: request.params.scheduleEventId, user: request.user._id}, {
                DayOfTheWeek: request.body.DayOfTheWeek,
                StartTime: request.body.StartTime,
                EndTime: request.body.EndTime,
                Description: request.body.Description,
                Color: request.body.Color,
                user: request.session.userId
            }).then(() => {
                response.set("Access-Control-Allow-Origin", "*");
                response.status(200).send("updated");
            }).catch((error) => {
                if (error.errors){
                    var errorMessages = {};
                    for (var fieldName in error.errors){
                        errorMessages[fieldName] = error.errors[fieldName].message
                    }
                    console.error("failed to update event with ID: ", request.params.scheduleEventId);
                    response.sendStatus(404).json(errorMessages);
                } else {
                    response.status(500).send("Unknown error: ", error);
                }
            });
        } else {
            console.log("event with that id not found");
            response.sendStatus(404);
        }
    }).catch((error) => {
        console.error("failed to query event with ID: ", request.params.scheduleEventId);
        response.sendStatus(404);
    });
});

app.delete("/scheduleEvents/:scheduleEventId", authorizeRequest, function (request, response) {
    console.log("at server, delete event with id: ", request.params.scheduleEventId);
    model.ScheduleEvent.deleteOne({_id: request.params.scheduleEventId, user: request.user._id}).then(() => {
        response.set("Access-Control-Allow-Origin", "*");
        response.status(200).send("deleted");
    }).catch((error) => {
        console.error("failed to delete event with ID: ", request.params.scheduleEventId);
        response.sendStatus(404);
    });
});

const lastEinkUpdateSentTime = {}; 
const einkCooldownDuration = 30 * 1000;

app.get("/einkScheduleDisplay", authorizeRequest, function (request, response){
    const userId = request.session.userId;
    const currentTime = Date.now();

    if (lastEinkUpdateSentTime[userId] && (currentTime - lastEinkUpdateSentTime[userId] < einkCooldownDuration)) {
        const timeRemaining = (einkCooldownDuration - (currentTime - lastEinkUpdateSentTime[userId])) / 1000;
        console.log(`update blocked for user ${userId}, wait ${timeRemaining.toFixed(1)} seconds`);
        response.set("Access-Control-Allow-Origin", "*");
        return response.status(429).send(`${timeRemaining.toFixed(1)}`);
    }


    var userHeader = "";
    var userFooter = "";
    var userScheduleNodeId = "";
    var filter = { user: userId};
    model.ScheduleEvent.find(filter).populate('user').then((scheduleEvents) => {
        console.log("events to update e ink: ", scheduleEvents);
        model.User.findOne({_id: userId}).then(function (user) {
            userHeader = user.scheduleHeader;
            userFooter = user.scheduleFooter;
            userScheduleNodeId = user.scheduleNodeId;
        }).then(() => {
            const formattedData = {
                eInkId: userScheduleNodeId,
                messageType: "schedule",
                header: userHeader,
                footer: userFooter,
                events: scheduleEvents.map(event => ({
                    dayOfWeek: event.DayOfTheWeek,
                    startTime: event.StartTime,
                    endTime: event.EndTime,
                    description: event.Description,
                    color: event.Color
                }))
            };
            if (clients["esp32"]) {
                clients["esp32"].send(JSON.stringify(formattedData));
                console.log("message sent to client: ", formattedData);
                lastEinkUpdateSentTime[userId] = currentTime;
                response.set("Access-Control-Allow-Origin", "*");
                response.status(200).send("Done");
            } else {
                console.log("esp32 not connected, not sent", formattedData);
                response.set("Access-Control-Allow-Origin", "*");
                response.status(422).send("No ESP32");
            }
        }).catch((error) => {
            console.error(error);
        });
    }).catch(error => {
        response.status(404).send("Mongoose Error: ", error);
    });
});

app.put("/me", authorizeRequest, function (request, response) {
    model.User.findOneAndUpdate({_id: request.user._id}, {
        firstName: request.body.firstName,
        lastName: request.body.lastName,
        scheduleNodeId: request.body.scheduleNodeId,
        statusNodeId: request.body.statusNodeId,
        scheduleHeader: request.body.scheduleHeader,
        scheduleFooter: request.body.scheduleFooter
    }).then(function (user) {
        response.set("Access-Control-Allow-Origin", "*");
        response.status(200).send("updated");
    }).catch((error) => {
        if (error.errors){
            var errorMessages = {};
            for (var fieldName in error.errors){
                errorMessages[fieldName] = error.errors[fieldName].message
            }
            console.error("failed to update user with ID: ", request.user._id);
            response.sendStatus(404).json(errorMessages);
        } else {
            response.status(500).send("Unknown error: ", error);
        }
    });
});


app.post("/users", function (request, response) {
    console.log("Request body:", request.body);

    const newUser = new model.User({
        firstName: request.body.firstName,
        lastName: request.body.lastName,
        email: request.body.email,
        scheduleNodeId: request.body.scheduleNodeId,
        statusNodeId: request.body.statusNodeId,
        scheduleHeader: request.body.scheduleHeader,
        scheduleFooter: request.body.scheduleFooter
    });

    newUser.setEncryptedPassword(request.body.plainPassword)
        .then(function () { 
            newUser.save()
                .then(() => {
                    response.status(201).send("created");
                })
                .catch((error) => {
                    if (error.errors) {
                        var errorMessages = {};
                        for (var fieldName in error.errors) {
                            errorMessages[fieldName] = error.errors[fieldName].message;
                        }
                        response.status(422).json(errorMessages);
                    } else if (error.code == 11000) {
                        response.status(422).json({ email: "User with email already exists" });
                    } else {
                        console.log("Unknown error creating user:", error);
                        response.status(500).send("Unknown error creating User.");
                    }
                });
        })
        .catch((error) => {
            console.log("Error setting encrypted password:", error);
            response.status(500).send("Error encrypting password.");
        });
});

//user authentication
app.get("/session", authorizeRequest, function (request, response){
    console.log("session: ", request.session);
    response.json(request.user);
    response.status(200);
});

app.post("/session", function (request, response){
    model.User.findOne({email: request.body.email}).then(function (user){
        if (user) {
            user.verifyEncryptedPassword(request.body.plainPassword).then(function (match){
                if (match) {
                    request.session.userId = user._id;
                    response.status(201).send("Authenticated");
                } else {
                    response.status(401).send("Not Authenticated");
                }
            })
        } else {
            response.status(401).send("Not Authentication");
        }
    }).catch((error) => {
        console.log("failed to find user for session, error: ", error);
    });
});

app.delete("/session", authorizeRequest, function (request,response){
    request.session.userId = null;
    response.status(200).send("logged out");
});

const server = app.listen(port, function(){
    console.log(`HTTP server listening on port ${port}`);
    console.log(`web socket server running on ws://${server.address().address === '::' ? 'localhost' : server.address().address}:${port}`);
});

const wss = new WebSocket.WebSocketServer({ server: server });
const clients = {};
function sendMessageToClient(clientId, message) {
    if (clients[clientId]) {
        clients[clientId].send(message);
        console.log("message sent to client: ", clientId);
        return true;
    } else {
        return false;
    }
};

wss.on('connection', function connection(ws) {
    console.log("clients: ", Object.keys(clients));

    ws.on('error', function (error) {
        ws.on('error', console.error);
    });

    ws.on('message', function message(message) {
        const jsonString = message.toString('utf-8');
        const data = JSON.parse(jsonString);
        console.log('Received:', data);
        if (data.type === "identify") {
            clients[data.clientId] = ws;
        } else if(data.type === "loraMessage") {
            sendMessageToClient("esp32", data.message);
        }
        console.log("clients: ", Object.keys(clients));
    });
});