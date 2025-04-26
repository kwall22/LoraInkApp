

let app = Vue.createApp({
    data: function(){
        return {
            messageInput: "",
            fillerStatuses:  ["", "", ""],
            showRegisterModal: false,
            showLoginModal: false,
            showAccountPage: false,
            showHomePage: true,
            showSchedulePage: false,
            showAddEventModal: false,
            showModifyEventModal: false,
            registerData: {
                firstName: "",
                lastName: "",
                email: "",
                password: "",
                scheduleNodeId: "",
                statusNodeId: "",
                scheduleHeader: "",
                scheduleFooter: "",
            },
            registrationErrorMessage: "",
            registrationError: false,
            loginData: {
                email: "",
                password: "",
            },
            loginErrorMessage: "",
            loginError: false,
            loggedIn: false,
            userData: {
                firstName: "",
                lastName: "",
                email: "",
                scheduleNodeId: "",
                statusNodeId: "",
                scheduleHeader: "",
                scheduleFooter: "",
            },
            updateUserData: {
                firstName: "",
                lastName: "",
                scheduleNodeId: "",
                statusNodeId: "",
                scheduleHeader: "",
                scheduleFooter: "",
            },
            eventForm: {
                selectedDays: [],
                startTime: "",
                endTime: "",
                description: "",
                color: "",
            },
            daysList: ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"],
            eventSchedule: [],
            statusMessages: [],
            tableStartTime: 8,
            tableEndTime: 20,
            tableIncrementMinutes: 60,
            timeSlots: [],
            takenCells: [],
            updateEventForm: {
                _id: "",
                dayOfTheWeek: "",
                startTime: "",
                endTime: "",
                description: "",
                color: "",
            },
            waitOnUpdateSchedule: false,
            waitMessage: "",
            scheduleESPNotConnected: false,
            scheduleESPNotConnectedMessage: "",
            scheduleChangesMade: false,
        }
    },
    methods: {
        toHomePage: function() {
            this.showHomePage = true;
            this.showAccountPage = false;
            this.showSchedulePage = false;
        },
        toAccountPage: function() {
            this.showAccountPage = true;
            this.showHomePage = false;
            this.showSchedulePage = false;
        },
        toRegisterModal: function() {
            this.clearRegisterFields();
            this.showRegisterModal = true;
            this.showLoginModal = false;
        },
        toLoginModal: function () {
            this.showLoginModal = true;
            this.showRegisterModal = false;
        },
        toSchedulePage: function (){
            this.refillEventsToDisplay();
            this.showSchedulePage = true;
            this.showHomePage = false;
            this.showAccountPage = false;
        },
        getStatusText: function (index) {
            if (index < this.statusMessages.length) {
              return this.statusMessages[index].Content;
            } else {
              const fillerIndex = index - this.statusMessages.length;
              return this.fillerStatuses[fillerIndex];
            }
        },
        getAllStatusMessages: function (){
            if (!this.loggedIn){
                console.log("not logged in, no status");
                return;
            }
            fetch("/statusMessages").then((response) => {
                if(response.status == 200) {
                    response.json().then((statusMessagesFromServer) => {
                        console.log("received status messages from server: ", statusMessagesFromServer);
                        statusMessagesFromServer.sort((a, b) => new Date(b.LastUsed) - new Date(a.LastUsed));
                        this.statusMessages = statusMessagesFromServer;

                    });
                }
            });
        },

        sendMessageToServer: function (){
            if (!this.loggedIn){
                console.log("not logged in");
                return "";
            }
            var messageToSend = "";
            if (arguments.length > 0 && typeof arguments[0] === "string") {
                console.log("Arg1:", arguments[0]);
                messageToSend = arguments[0];
            } else {
                if(!this.messageInput){
                    let sendButton = this.$refs.sendMessageButton;
                    sendButton.classList.add("shake");
                    setTimeout(() => {
                        sendButton.classList.remove("shake");
                    }, 300);
                    return;
                }
                messageToSend = this.messageInput;
                this.messageInput = "";
            }
            console.log("message to send: ", messageToSend);
            const foundStatusMessageObject = this.statusMessages.find(item => item.Content === messageToSend);
            if (foundStatusMessageObject){
                console.log("found existing status, sending to update: ", foundStatusMessageObject);
                this.updateStatusMessage(foundStatusMessageObject);
                return "";
            }
            const now = new Date();
            const currentDate = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')}T${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
            var data = "Content=" + encodeURIComponent(messageToSend);
            data += "&LastUsed=" + encodeURIComponent(currentDate);
            fetch("/statusMessages", {
                method: "POST",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                console.log("response.status: ", response.status);
                if (response.status == 201) {
                    console.log("status posted");
                    this.getAllStatusMessages();
                } else {
                    console.log("error: ", response.status);
                }
            }).catch((error) => {
                console.log("error in the catch: ", error);
            });
        },
        updateStatusMessage: function (statusMessageObject) {
            const now = new Date();
            const currentDate = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')}T${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
            var statusMessageId = statusMessageObject._id;
            var data = "Content=" + encodeURIComponent(statusMessageObject.Content);
            data += "&LastUsed=" + encodeURIComponent(currentDate);
            console.log("id of status message to update: ", statusMessageId);
            fetch("/statusMessages/"+ statusMessageId, {
                method: "PUT",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                if (response.status == 200) {
                    console.log("update successful!");
                    this.messageInput = "";
                    this.getAllStatusMessages();
                } else {
                    console.log("update not successful");
                }
            });
        },
        deleteStatusMessage: function (statusMessageId) {
            if (!this.loggedIn){
                return "not logged in";
            }
            fetch("/statusMessages/" + statusMessageId, {
                method: "DELETE"
            }).then((response) => {
                if (response.status == 200) {
                    console.log("successful delete");
                    this.getAllStatusMessages();
                }
            });
        },
        validateDeviceId(deviceId) {
            const hexRegex = /^[0-9a-fA-F]+$/;
            var isValid = hexRegex.test(deviceId);
            if (isValid) {
                return true;
            } else {
                return false;
            }
        },
        clearRegisterFields(){
            this.registrationErrorMessage = "";
            this.registrationError = false;
            this.registerData.firstName = "";
            this.registerData.lastName = "";
            this.registerData.email = "";
            this.registerData.password = "";
            this.registerData.scheduleNodeId = "";
            this.registerData.statusNodeId = "";
            this.registerData.scheduleHeader = "";
            this.registerData.scheduleFooter = "";
            return;
        },
        registerUser: function () {
            if (!this.registerData.firstName || !this.registerData.lastName || !this.registerData.email || !this.registerData.password || !this.registerData.scheduleNodeId || !this.registerData.statusNodeId || !this.registerData.scheduleHeader || !this.registerData.scheduleFooter)  {
                this.registrationErrorMessage = "All fields required";
                this.registrationError = true;
                let registerButton = this.$refs.registerUserButton;
                registerButton.classList.add("shake");
                setTimeout(() => {
                    registerButton.classList.remove("shake");
                }, 300);
                return;
            } 
            if (this.registerData.scheduleNodeId == this.registerData.statusNodeId) {
                this.registrationErrorMessage = "Device ID's must be different"
                this.registrationError = true;
                let registerButton = this.$refs.registerUserButton;
                registerButton.classList.add("shake");
                setTimeout(() => {
                    registerButton.classList.remove("shake");
                }, 300);
                return;
            }
            if (!this.validateDeviceId(this.registerData.scheduleNodeId) || !this.validateDeviceId(this.registerData.statusNodeId)) {
                this.registrationErrorMessage = "Device ID's must be valid HEX codes"
                this.registrationError = true;
                let registerButton = this.$refs.registerUserButton;
                registerButton.classList.add("shake");
                setTimeout(() => {
                    registerButton.classList.remove("shake");
                }, 300);
                return;
            }
        
            var data = "firstName=" + encodeURIComponent(this.registerData.firstName);
            data += "&lastName=" + encodeURIComponent(this.registerData.lastName);
            data += "&email=" + encodeURIComponent(this.registerData.email);
            data += "&plainPassword=" + encodeURIComponent(this.registerData.password);
            data += "&scheduleNodeId=" + encodeURIComponent(this.registerData.scheduleNodeId);
            data += "&statusNodeId=" + encodeURIComponent(this.registerData.statusNodeId);
            data += "&scheduleHeader=" + encodeURIComponent(this.registerData.scheduleHeader);
            data += "&scheduleFooter=" + encodeURIComponent(this.registerData.scheduleFooter);
            fetch("/users", { 
                method: "POST",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                if (response.status == 201) {
                    console.log("Successful registration!");
                    this.registrationErrorMessage = "";
                    this.registrationError = false;
                    this.clearRegisterFields();
                } else {
                    console.log("Error: ", response.status);
                    response.json().then(errorJSON => {
                        console.log("Error message: ", errorJSON);
                        if (errorJSON.email === "User with email already exists") {
                            this.registrationErrorMessage = "Email address already in use";
                            this.registrationError = true;
                        } else if (errorJSON.email && errorJSON.email.includes("is not a valid email address")) {
                            this.registrationErrorMessage = "Please enter a valid Email address";
                            this.registrationError = true;
                        }
                    });
                }
            }).catch((error) => {
                console.log("Error in the catch: ", error);
            });
        },
        clearLoginFields(){
            this.loginData.email = "";
            this.loginData.password = "";
            this.loginErrorMessage = "";
            this.loginError = false;
            return;
        },
        loginUser: function (){
            if (this.loggedIn){
                return;
            }
            if (!this.loginData.email || !this.loginData.password) {
                this.loginErrorMessage = "All fields required";
                this.loginError = true;
                let loginButton = this.$refs.loginUserButton;
                loginButton.classList.add("shake");
                setTimeout(() => {
                    loginButton.classList.remove("shake");
                }, 300);
                return;
            }
            var data = "email=" + encodeURIComponent(this.loginData.email);
            data += "&plainPassword=" + encodeURIComponent(this.loginData.password);
            fetch("/session", {
                method: "POST",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                if(response.status == 201){
                    this.loggedIn = true;
                    this.loginErrorMessage = "";
                    this.loginError = false;
                    this.clearLoginFields();
                    this.checkLoggedIn();
                    console.log("successful logging in!");
                } else if(response.status == 401) {
                    this.loggedIn = false;
                    this.loginErrorMessage = "Incorrect email or password";
                    this.loginError = true;
                    let loginButton = this.$refs.loginUserButton;
                    loginButton.classList.add("shake");
                    setTimeout(() => {
                        loginButton.classList.remove("shake");
                    }, 300);
                }
            }).catch((error) => {
                this.loggedIn = false;
                console.log("fetch error: ", error);
            });
        },
        updateUserInformation: function () {
            if(!this.loggedIn){
                return;
            } 
            if(!this.updateUserData.firstName && !this.updateUserData.lastName && !this.updateUserData.scheduleNodeId && !this.updateUserData.statusNodeId && !this.updateUserData.scheduleHeader && !this.updateUserData.scheduleFooter ){
                let saveButton = this.$refs.saveUserButton;
                saveButton.classList.add("shake");
                setTimeout(() => {
                    saveButton.classList.remove("shake");
                }, 300);
                return;
            }
            if(!this.updateUserData.firstName) {
                this.updateUserData.firstName = this.userData.firstName;
            }
            if(!this.updateUserData.lastName) {
                this.updateUserData.lastName = this.userData.lastName;
            }
            if(!this.updateUserData.scheduleNodeId) {
                this.updateUserData.scheduleNodeId = this.userData.scheduleNodeId;
            }
            if(!this.updateUserData.statusNodeId) {
                this.updateUserData.statusNodeId = this.userData.statusNodeId;
            }
            if( (this.updateUserData.scheduleHeader && this.updateUserData.scheduleHeader != this.userData.scheduleHeader) || (this.updateUserData.scheduleFooter && this.updateUserData.scheduleFooter != this.userData.scheduleFooter)){
                this.scheduleChangesMade = true;
            }
            if(!this.updateUserData.scheduleHeader) {
                this.updateUserData.scheduleHeader = this.userData.scheduleHeader;
            }
            if(!this.updateUserData.scheduleFooter) {
                this.updateUserData.scheduleFooter = this.userData.scheduleFooter;
            }
            var data = "firstName=" + encodeURIComponent(this.updateUserData.firstName);
            data += "&lastName=" + encodeURIComponent(this.updateUserData.lastName);
            data += "&scheduleNodeId=" + encodeURIComponent(this.updateUserData.scheduleNodeId);
            data += "&statusNodeId=" + encodeURIComponent(this.updateUserData.statusNodeId);
            data += "&scheduleHeader=" + encodeURIComponent(this.updateUserData.scheduleHeader);
            data += "&scheduleFooter=" + encodeURIComponent(this.updateUserData.scheduleFooter);
            fetch("/me", {
                method: "PUT",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                if(response.status == 200) {
                    console.log("update successful!");
                    this.updateUserData = {
                        firstName: "",
                        lastName: "",
                        scheduleNodeId: "",
                        statusNodeId: "",
                        scheduleHeader: "",
                        scheduleFooter: "",
                    };
                    this.checkLoggedIn();
                } else {
                    console.log("update not successful");
                }
            });
        },
        logOutUser: function () {
            if(!this.loggedIn){
                return;
            }
            fetch("/session", {
                method: "DELETE"
            }).then((response) => {
                if(response.status == 200){
                    this.loggedIn = false;
                    this.userData = {};
                    this.statusMessages = [];
                    this.eventSchedule = [];
                    this.toHomePage();
                    console.log("successful log out");
                } else {
                    console.log("error logging out");
                }
            }).catch((error) => {
                console.log("fetch error logging out: ", error);
            });
        },
        checkLoggedIn: function(){
            console.log("checking logged in");
            fetch("/session").then((response)=> {
                if (response.status == 200){
                    this.loggedIn = true;
                    response.json().then((userInfo) => {
                        console.log("user info received: ", userInfo);
                        this.userData.firstName = userInfo.firstName;
                        this.userData.lastName = userInfo.lastName;
                        this.userData.email = userInfo.email;
                        this.userData.scheduleNodeId = userInfo.scheduleNodeId;
                        this.userData.statusNodeId = userInfo.statusNodeId;
                        this.userData.scheduleHeader = userInfo.scheduleHeader;
                        this.userData.scheduleFooter = userInfo.scheduleFooter; 
                    });
                    this.getAllStatusMessages();
                    this.getAllScheduledEvents();
                } else {
                    this.loggedIn = false;
                }
            }).catch((error) => {
                this.loggedIn = false;
                console.log("fetch error: ", error);
            });
        },
        addEvent: function () {
            if (!this.loggedIn){
                console.log("not logged in");
                return "";
            }
            console.log("event input: ", this.eventForm);
            if(!this.eventForm || this.eventForm.description == "" || this.eventForm.startTime == "" || this.eventForm.endTime == "" || this.eventForm.selectedDays == [] || this.eventForm.color == ""){
                let addButton = this.$refs.addEventButton;
                addButton.classList.add("shake");
                setTimeout(() => {
                    addButton.classList.remove("shake");
                }, 300);
                console.log("bad event");
                return;
            }
            this.eventForm.selectedDays.forEach(day => {
                this.eventSchedule.push({
                  dayOfTheWeek: day,
                  startTime: this.eventForm.startTime,
                  endTime: this.eventForm.endTime,
                  description: this.eventForm.description,
                  color: this.eventForm.color
                });
                var data = "DayOfTheWeek=" + encodeURIComponent(day);
                data += "&StartTime=" + encodeURIComponent(this.eventForm.startTime);
                data += "&EndTime=" + encodeURIComponent(this.eventForm.endTime);
                data += "&Description=" + encodeURIComponent(this.eventForm.description);
                data += "&Color=" + encodeURIComponent(this.eventForm.color);
                console.log("data to be sent to server: ", data);
                fetch("/scheduleEvents", {
                    method: "POST",
                    body: data,
                    headers: {
                        "Content-Type": "application/x-www-form-urlencoded"
                    }
                }).then((response) => {
                    if (response.status == 201) {
                        console.log("added event!");
                        this.getAllScheduledEvents();
                        this.scheduleChangesMade = true;
                    }
                })
            });
        
            this.clearAddEventFields();
            this.toggleAddEventModal(0,0);
        },
        clearAddEventFields: function(){
            this.eventForm.description = '';
            this.eventForm.startTime = '';
            this.eventForm.endTime = '';
            this.eventForm.selectedDays = [];
            this.eventForm.color = '';
        },
        prefillAddEventFields: function (index, dayNumber){
            var timeSlotStartTime = this.tableStartTime + (index - 2);
            var timeSlotEndTime = this.tableStartTime + (index - 1);
            var timeSlotStartString = timeSlotStartTime.toString().padStart(2, '0');
            var timeSlotEndString = timeSlotEndTime.toString().padStart(2, '0');
            this.eventForm.startTime = `${timeSlotStartString}:00`;
            this.eventForm.endTime = `${timeSlotEndString}:00`;
            this.eventForm.selectedDays.push(dayNumber);
        },
        toggleAddEventModal: function (index, dayNumber) {
            if (index == 0 && dayNumber == 0){
                this.showAddEventModal = false;
                this.clearAddEventFields();
                return '';
            }
            const startRow = 2;
            const startCol = 1;
            const totalColumns = 5;
            this.prefillAddEventFields(index, dayNumber);
            this.$nextTick(() => {
                let refSlot = this.$refs.slot;
                const slotIndex = (index - startRow) * totalColumns + (dayNumber - startCol);
                let timeSlotEl = refSlot[slotIndex];
                console.log(" time slot: ", timeSlotEl);
                console.log("slot index: ", slotIndex);
                console.log("index: ", index);
                console.log("dayNumber", dayNumber)
                this.refillEventsToDisplay();
                this.$nextTick(() => {
                    let eventEl = timeSlotEl.querySelector('.event');
                    if (eventEl) {
                        console.log("Event exists! Class name is: ", eventEl.className);
                        if (eventEl.classList.contains('event')) {
                            console.log("It has the 'event' class");
                            this.showAddEventModal = false;
                            this.showModifyEventModal = true;
                        }
                    } else {
                        console.log(" No event element found in this slot");
                        if(this.showAddEventModal) {
                            this.showAddEventModal = false;
                        } else {
                            this.showAddEventModal = true;
                            this.showModifyEventModal = false;
                        }
                    }
                })
                
            });
        },
        prefillUpdateEventFields: function (event){
            this.updateEventForm = {
                _id: event._id,
                dayOfTheWeek: this.formatDay(event.dayOfTheWeek),
                startTime: event.startTime,
                endTime: event.endTime,
                description: event.description,
                color: event.color
            };
            this.showModifyEventModal = true;

        },
        updateEvent: function () {
            if (this.updateEventForm._id == "" || this.updateEventForm.dayOfTheWeek == "" || this.updateEventForm.startTime == "" || this.updateEventForm.endTime == "" || this.updateEventForm.description == "" || this.updateEventForm.color == ""){
                console.log("no update info");
                let updateButton = this.$refs.updateEventButton;
                updateButton.classList.add("shake");
                setTimeout(() => {
                    updateButton.classList.remove("shake");
                }, 300);
                return "";
            }
            var scheduleEventId = this.updateEventForm._id;
            console.log("id of event to update: ", scheduleEventId);
            const numberedDayOfWeek = this.daysList.indexOf(this.updateEventForm.dayOfTheWeek);
            if (numberedDayOfWeek == -1) {
                console.log("problem getting day of the week");
                return "";
            }
            var data = "DayOfTheWeek=" + encodeURIComponent(numberedDayOfWeek);
            data += "&StartTime=" + encodeURIComponent(this.updateEventForm.startTime);
            data += "&EndTime=" + encodeURIComponent(this.updateEventForm.endTime);
            data += "&Description=" + encodeURIComponent(this.updateEventForm.description);
            data += "&Color=" + encodeURIComponent(this.updateEventForm.color);
            console.log("data to be sent to the server to update event: ", data);
            fetch("/scheduleEvents/" + scheduleEventId, {
                method: "PUT",
                body: data,
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                }
            }).then((response) => {
                if(response.status == 200) {
                    console.log("update successful!");
                    this.updateEventForm = {
                        _id: "",
                        dayOfTheWeek: "",
                        startTime: "",
                        endTime: "",
                        description: "",
                        color: ""
                    };
                    this.getAllScheduledEvents();
                    this.closeModifyEventModal();
                    this.scheduleChangesMade = true;
                } else {
                    console.log("update not successful");
                }
            })
        },
        deleteEvent: function () {
            if (this.updateEventForm._id == ""){
                console.log("empty id, cant delete");
                return "";
            }
            var scheduleEventId = this.updateEventForm._id;
            console.log("id of event to delete: ", scheduleEventId);
            fetch("/scheduleEvents/" + scheduleEventId, {
                method: "DELETE"
            }).then((response) => {
                if (response.status == 200) {
                    console.log("successful delete!");
                    this.updateEventForm = {
                        _id: "",
                        dayOfTheWeek: "",
                        startTime: "",
                        endTime: "",
                        description: "",
                        color: ""
                    };
                    this.getAllScheduledEvents();
                    this.closeModifyEventModal();
                    this.scheduleChangesMade = true;
                }
            }).catch((error) => {
                console.log("bad delete, error: ", error);
            })
        },
        closeModifyEventModal: function (){
            this.showModifyEventModal = false;
            this.refillEventsToDisplay();
        },
        getCheckedValue: function (dayNumber, dayOfTheWeek) {
            if (dayNumber == dayOfTheWeek){
                return "checked";
            } else {
                return "";
            }
        },
        getAllScheduledEvents: function (){
            if (!this.loggedIn){
                console.log("not logged in, no events");
                return;
            }
            fetch("/scheduleEvents").then((response) => {
                if(response.status == 200) {
                    response.json().then((scheduleEventsFromServer) => {
                        console.log("received events from server: ", scheduleEventsFromServer);
                        this.eventSchedule = [];
                        let earliestTime = "08:00";
                        let latestTime = "20:00";
                        scheduleEventsFromServer.forEach(e => {
                            if(e.StartTime < earliestTime){
                                earliestTime = e.StartTime;
                            }
                            if(e.EndTime > latestTime){
                                latestTime = e.EndTime;
                            }
                            this.eventSchedule.push({
                                _id: e._id,
                                dayOfTheWeek: e.DayOfTheWeek,
                                startTime: e.StartTime,
                                endTime: e.EndTime,
                                description: e.Description,
                                color: e.Color
                            });
                        });
                        this.eventsLeftToDisplay = [...this.eventSchedule];
                        this.tableStartTime = parseInt(earliestTime.split(":")[0]);
                        this.tableEndTime = parseInt(latestTime.split(":")[0])
                        console.log(this.tableStartTime, this.tableEndTime);
                        this.generateTimeSlots();
                    });
                }
            });
        },
        refillEventsToDisplay: function () {
            console.log("refilling, not fetching");
            this.eventsLeftToDisplay = [];
            this.eventsLeftToDisplay = [...this.eventSchedule];
            this.generateTimeSlots();
        },
        updateEInkSchedule: function() {
            if (!this.loggedIn){
                console.log("not logged in");
                return "";
            }
            fetch("/einkScheduleDisplay").then((response) => {
                if(response.status == 200) {
                    this.scheduleESPNotConnected = false;
                    this.scheduleESPNotConnectedMessage = "";
                    this.waitMessage = "";
                    this.waitOnUpdateSchedule = false;
                    this.scheduleChangesMade = false;
                    console.log("server successfully sent the full schedule to the esp32");
                } else if (response.status == 422){
                    this.scheduleESPNotConnected = true;
                    this.scheduleESPNotConnectedMessage = "Not Connected";
                    console.log("not connected to esp32");
                } else if (response.status == 429) {
                    console.log( "too many request!! wait at least 60 seconds!");
                    response.text().then(errorMessage => {
                        console.log("Error message from server:", errorMessage);
                        this.scheduleESPNotConnected = false;
                        this.scheduleESPNotConnectedMessage = "";
                        this.waitMessage = errorMessage;
                        this.waitOnUpdateSchedule = true;

                        let updateEInkScheduleButton = this.$refs.updateEInkScheduleButton;
                        updateEInkScheduleButton.disabled = true;
                        var waitTime = parseInt(this.waitMessage);
                        console.log("wait time: ", waitTime);
                        setTimeout(() => {
                            this.waitOnUpdateSchedule = false;
                            updateEInkScheduleButton.disabled = false;
                            this.waitMessage = "";
                            console.log("wait time over");
                        }, waitTime * 1000);
                    });
                    
                } else {
                    console.log("other response from server: ", response.statusText);
                }
            });
        },
        formatDay: function(dayNumber){
            return this.daysList[dayNumber];
        }, 
        formatTime: function(time){
            var parts = time.split(":");
            var hour = parseInt(parts[0]);
            var minutes = parts[1];
            if (hour > 12){
                hour = hour - 12;
            }
            return `${hour}:${minutes}`
        },
        generateTimeSlots: function() {
            const startHour = this.tableStartTime;
            const endHour = this.tableEndTime;
            const increment = this.tableIncrementMinutes;
            let slots = [];
            for (let h = startHour; h <= endHour; h++) {
                for (let m = 0; m < 60; m += increment) {
                    slots.push(`${String(h)}:${String(m).padStart(2, '0')}`);
                    
                }
            }
            console.log(slots);
            this.timeSlots = [...slots];
        },
        timeToGridRow(unformattedTime) {
            const time = this.timeStringToMinutes(unformattedTime);
            const totalMinutes = time - (this.tableStartTime * 60);
            const row = Math.floor(totalMinutes / this.tableIncrementMinutes) + 2;
            return row;
        },
        getRGBforEvent(colorString){
            if (colorString == "RED"){
                return 'rgb(181, 42, 42)';
            } if (colorString == "YELLOW") {
                return 'rgb(220, 227, 32)';
            } if (colorString == "ORANGE"){
                return 'rgb(184, 112, 18)';
            } if (colorString == "GREEN") {
                return 'rgb(37, 128, 49)';
            } if (colorString == "BLUE"){
                return 'rgb(38, 47, 130)';
            } else{
                return 'rgb(37, 128, 49)';
            }
        },
        getTextColorForEvent(colorString) {
            if (colorString == "YELLOW" || colorString == "ORANGE") {
                return 'black';
            } else {
                return 'white';
            }
        },
        getEventStyle(event) {
            const startTimeMinutes = this.timeStringToMinutes(event.startTime);
            const endTimeMinutes = this.timeStringToMinutes(event.endTime);
            const incrementMinutes = this.tableIncrementMinutes;
            const startOffsetMinutes = startTimeMinutes % incrementMinutes;
            const eventHeight = (endTimeMinutes - startTimeMinutes) / incrementMinutes;
            const topOffset = startOffsetMinutes / incrementMinutes;
            return {
                top: `${topOffset * 100}%`,
                height: `${eventHeight * 100}%`,
                backgroundColor: this.getRGBforEvent(event.color),
                color: this.getTextColorForEvent(event.color)
            };
        },
        timeStringToMinutes: function (timeString) {
            const [hours, minutes] = timeString.split(":").map(Number);
            return hours * 60 + minutes;
        },
        isCellTaken: function(row, dayNumber){
            return this.takenCells.some(tuple => tuple[0] === row && tuple[1] === dayNumber) ? { display: 'none' } : { display: 'block' };
            
        },
        generateTakenCells: function() {
            this.takenCells = [];
            this.eventSchedule.forEach(event => {
                const rowStart = this.timeToGridRow(event.startTime);
                const rowEnd = this.timeToGridRow(event.endTime);
                for (let i = rowStart; i < rowEnd; i++) {
                    this.takenCells.push([i, event.dayOfTheWeek]);
                }
            });
            console.log("taken cells: ", this.takenCells);
        }
    },
    computed: {
        scheduleStyle() {
            const startTimeMinutes = this.tableStartTime * 60;
            const endTimeMinutes = this.tableEndTime * 60;
            const numRows = (endTimeMinutes - startTimeMinutes) / this.tableIncrementMinutes;
            console.log("num rows:", numRows);
            return {
                gridTemplateRows: `auto repeat(${numRows}, 1fr)`,
                '--num-rows': numRows,
            };
        }
    },
    created: function(){
        console.log("created function ran");
        this.checkLoggedIn();
    }
}).mount("#app");