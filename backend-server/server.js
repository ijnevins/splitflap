// server.js
const express = require('express');
const cors = require('cors'); 
const dotenv = require('dotenv');
const path = require('path');
const admin = require('firebase-admin');
const mqtt = require('mqtt');

// Load environment variables (optional, but good practice for future keys)
dotenv.config();

// --- 1. FIREBASE INITIALIZATION (Unchanged) ---
try {
    // 1. Get the JSON string from the environment variable
    const serviceAccountJson = process.env.FIREBASE_SERVICE_ACCOUNT;
    
    if (!serviceAccountJson) {
        throw new Error('FIREBASE_SERVICE_ACCOUNT environment variable not set.');
    }
    
    // 2. Parse the string back into a JavaScript object
    const serviceAccount = JSON.parse(serviceAccountJson); 
    
    admin.initializeApp({
      credential: admin.credential.cert(serviceAccount)
    });
    console.log('Firebase Admin SDK Initialized!');
} catch (error) {
    // If running locally without the ENV var, this will fail.
    console.error('ERROR: Firebase configuration failed.', error.message);
    process.exit(1);
}

const db = admin.firestore();
const app = express();
const PORT = process.env.PORT || 3000;

// --- 2. MIDDLEWARES (Unchanged) ---
app.use(cors()); 
app.use(express.json());
app.use((req, res, next) => {
  res.set('Content-Type', 'application/json; charset=utf-8');
  next();
});

app.use(express.static(path.join(__dirname, '..', 'frontend')));


// --- 3. CORRECTED HELPER FUNCTION ---
// Fetches the last 20 messages, sorted oldest to newest
async function getHistoryFromFirestore() {
    const MAX_HISTORY = 20;
    
    // 1. Get the NEWEST 20 messages first
    const snapshot = await db.collection('messages')
        .orderBy('time', 'desc') // DESC = Newest first
        .limit(MAX_HISTORY)
        .get();

    const history = [];
    snapshot.forEach(doc => {
        const data = doc.data();
        
        // Convert timestamp securely
        if (data.time && data.time.toDate) {
            data.time = data.time.toDate().toISOString(); 
        } else {
            data.time = new Date().toISOString(); 
        }

        history.push({ 
            id: doc.id,
            ...data 
        });
    });

    // 2. Reverse the array so the Client displays them Oldest -> Newest (Chat style)
    return history.reverse();
}


// --- 4. API ENDPOINTS (Unchanged) ---

app.get('/', (req, res) => {
    // This provides an immediate response without hitting Firebase
    res.status(200).json({ status: "API is awake and running" });
});

// GET /api/messages - Load all messages (initial load)
app.get('/api/messages', async (req, res) => {
    try {
        const history = await getHistoryFromFirestore();
        res.status(200).json(history);
    } catch (error) {
        console.error("Error fetching history:", error);
        res.status(500).json({ message: 'Failed to load history' });
    }
});


// POST /api/messages - Save a new message (on send button click)
app.post('/api/messages', async (req, res) => {
    // 1. ADD 'sender' to this line so we actually read it
    const { message, target, sender } = req.body;

    if (!message || !target) {
        return res.status(400).json({ message: 'Missing message or target' });
    }

    try {
        // 2. Save the new message to Firestore including the Sender
        const newItem = {
            message: message,
            target: target,
            sender: sender || "Anonymous", // Fallback if name is missing
            // Use Firestore's reliable server timestamp
            time: admin.firestore.FieldValue.serverTimestamp() 
        };
        await db.collection('messages').add(newItem);

        // 3. Fetch and send the entire updated history back
        const history = await getHistoryFromFirestore(); 

        res.status(201).send(JSON.stringify(history));
        
    } catch (error) {
        console.error("Error saving message:", error);
        res.status(500).json({ message: 'Failed to save message' });
    }
});


// --- 5. START SERVER (Unchanged) ---
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
    console.log(`Access website at: http://localhost:${PORT}/website.html`);
});