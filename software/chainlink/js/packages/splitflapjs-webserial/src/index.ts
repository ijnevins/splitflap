import {MessageCallback, SplitflapCore} from 'splitflapjs-core'

// Assuming SerialPort is imported correctly, usually from splitflapjs-proto or standard types
// NOTE: You may need to manually add the correct import for SerialPort if it's not resolved.
// import { SerialPort } from 'splitflapjs-proto/dist/serial-protocol'; 

export class SplitflapWebSerial extends SplitflapCore {
    private port: SerialPort | null
    private writer: WritableStreamDefaultWriter<Uint8Array> | undefined = undefined

    constructor(port: SerialPort, onMessage: MessageCallback) {
        super(onMessage, (packet: Uint8Array) => {
            this.writer?.write(packet).catch(this.onError)
        })
        this.port = port
        this.portAvailable = true
        this.onStart();
        this.port.addEventListener('disconnect', () => {
            console.log('shutting down on disconnect')
            this.port = null
            this.portAvailable = false
        })
    }

    public async openAndLoop() {
        if (this.port === null) {
            return
        }
        
        console.log("DEBUG LOG: Opening port with baud rate:", SplitflapCore.BAUD); // <--- DEBUG
        await this.port.open({baudRate: SplitflapCore.BAUD})
        
        if (this.port.readable === null || this.port.writable === null) {
            throw new Error('Port missing readable or writable!')
        }

        // Keep your 500ms delay here if you added it previously
        await new Promise(resolve => setTimeout(resolve, 500)); 

        const reader = this.port.readable.getReader()
        try {
            const writer = this.port.writable.getWriter()
            
            // DEBUG: Log the handshake being sent
            const handshakeBytes = Uint8Array.from([0, 0, 0, 0, 0, 0, 0, 0]);
            console.log("DEBUG LOG: Sending Handshake Bytes:", handshakeBytes); 
            
            writer.write(handshakeBytes).catch(this.onError)
            this.writer = writer
            try {
                // eslint-disable-next-line no-constant-condition
                while (true) {
                    const {value, done} = await reader.read()
                    if (done) {
                        console.log("DEBUG LOG: Reader stream done/closed.");
                        break
                    }
                    if (value !== undefined) {
                        // DEBUG: Log exactly what we received from the ESP32
                        console.log("DEBUG LOG: Received RAW bytes:", value);
                        this.onReceivedData(value)
                    }
                }
            } finally {
                console.log('Releasing writer')
                writer?.releaseLock()
            }
        } finally {
            console.log('Releasing reader')
            reader.releaseLock()
        }
    }

    private onError(e: unknown) {
        console.error('Error writing serial', e)
        this.port?.close()
        this.port = null
        this.portAvailable = false
        this.writer = undefined
    }
}
