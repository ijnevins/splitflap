// models/Message.js
const mongoose = require('mongoose');

const MessageSchema = new mongoose.Schema({
    target: { type: String, required: true }, // 'Ian' or 'Eleri'
    message: { type: String, required: true, maxlength: 6 }, // The actual flap message
    time: { type: Date, default: Date.now }, // Timestamp
});

// Add an index for sorting performance (optional but good practice)
MessageSchema.index({ time: 1 });

module.exports = mongoose.model('Message', MessageSchema);