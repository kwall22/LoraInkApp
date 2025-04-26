const bcrypt = require('bcrypt');
const mongoose = require('mongoose');

mongoose.connect("", { // insert mongo db atlas address here
	}).then(() => console.log("Connected to MongoDB Atlas"))
  		.catch(err => console.error("Error connecting to MongoDB:", err));


const userSchema = new mongoose.Schema({
	firstName: {
		type: String,
		required: [true, "First name is required"]
	},
	lastName: {
		type: String,
		required: [true, "Last name is required"]
	},
	scheduleNodeId: {
		type: String,
		required: [true, "Node Id is required"]
	},
	statusNodeId: {
		type: String,
		required: [true, "Status Node Id is required"]
	},
	scheduleHeader: {
		type: String,
		required: [true, "Schedule Header is required"]
	},
	scheduleFooter: {
		type: String,
		required: [true, "Schedule Footer is required"]
	},
	email: {
		type: String,
		required: [true, "Email is required"],
		unique: true,
		validate: {
			validator: function (v) {
				return v.includes('@');
			},
			message: props => `${props.value} is not a valid email address.`
		}
	},
	encryptedPassword: {
		type: String,
		required: [true, "Password is required"],
		minlength: [6, "Password must be at least 6 characters long"]
	}
}, {
    toJSON: {
        versionKey: false,
        transform: function (doc, ret) {
            delete ret.encryptedPassword;
        }
    }
});

userSchema.methods.setEncryptedPassword = function (plainPassword) {
    return new Promise((resolve, reject) => {
        bcrypt.hash(plainPassword, 12).then(hash => {
            this.encryptedPassword = hash;
            resolve();
        }).catch(reject);
    });
};

userSchema.methods.verifyEncryptedPassword = function (plainPassword) {
    return bcrypt.compare(plainPassword, this.encryptedPassword);
};

const User = mongoose.model('User', userSchema);

const statusMessageSchema = new mongoose.Schema({
	Content: {
		type: String,
		required: [true, "Message Content is required"]
	},
	LastUsed: {
		type: Date,
		required: [true, "Last Used date is required"]
	},
	user: {
		type: mongoose.Schema.Types.ObjectId,
		ref: "User",
		required: [true, "Status Message must belong to a user"]
	}
});

const StatusMessage = mongoose.model('StatusMessage', statusMessageSchema);

const scheduleEventSchema = new mongoose.Schema({
	DayOfTheWeek: {
		type: Number,
		required: [true, "Day of the week is required"]
	},
	StartTime: {
		type: String,
		required: [true, "Start time is required"]
	},
	EndTime: {
		type: String,
		required: [true, "End time is required"]
	},
	Description: {
		type: String,
		required: [true, "Description is required"]
	},
	Color: {
		type: String,
		default: "GREEN"
	},
	user: {
		type: mongoose.Schema.Types.ObjectId,
		ref: "User",
		required: [true, "Schedule event must belong to a user"]
	}
});

const ScheduleEvent = mongoose.model('ScheduleEvent', scheduleEventSchema);

module.exports = {
	User: User,
	ScheduleEvent: ScheduleEvent,
	StatusMessage: StatusMessage
};