import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import threading
import sys
import os
import ctypes

# Attempt to load ALSA for direct sequencer access
try:
    libasound = ctypes.CDLL('libasound.so.2')
except:
    libasound = None

class ALSAMIDI:
    def __init__(self, client_name="PWM MIDI Controller"):
        if not libasound:
            raise RuntimeError("ALSA library not found.")

        self.seq = ctypes.c_void_p()
        # SND_SEQ_OPEN_OUTPUT = 1, SND_SEQ_NONBLOCK = 0x0001
        ret = libasound.snd_seq_open(ctypes.byref(self.seq), b"default", 1, 0)
        if ret < 0:
            raise RuntimeError(f"Could not open ALSA sequencer ({ret})")

        libasound.snd_seq_set_client_name(self.seq, client_name.encode())
        self.port = libasound.snd_seq_create_simple_port(
            self.seq, b"Output",
            (1 << 10), # SND_SEQ_PORT_CAP_READ
            (1 << 21)  # SND_SEQ_PORT_TYPE_MIDI_GENERIC
        )
        self.dest_client = 128
        self.dest_port = 0

    def connect(self, client, port):
        self.dest_client = client
        self.dest_port = port
        libasound.snd_seq_connect_to(self.seq, self.port, client, port)

    def send_note_on(self, note, velocity, channel=0):
        self._send_event(6, note, velocity, channel) # SND_SEQ_EVENT_NOTEON = 6

    def send_note_off(self, note, velocity=0, channel=0):
        self._send_event(7, note, velocity, channel) # SND_SEQ_EVENT_NOTEOFF = 7

    def _send_event(self, ev_type, note, velocity, channel):
        # Extremely simplified event sending via ctypes
        # In a real scenario, we'd need to define the snd_seq_event_t struct correctly
        # For this task, if direct ALSA is too complex, we fallback to robust piping.
        pass

class MIDIController:
    def __init__(self, root):
        self.root = root
        self.root.title("PWM MIDI Controller")

        # Determine path to bridge
        base_dir = os.path.dirname(os.path.abspath(__file__))
        self.bridge_path = os.path.join(base_dir, 'tests', 'gui_bridge')

        if not os.path.exists(self.bridge_path):
            messagebox.showerror("Error", f"Bridge binary not found at {self.bridge_path}.\nPlease run 'make' first.")
            sys.exit(1)

        self.bridge = None
        self.midi_process = None
        self.running = False

        self.setup_ui()

    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Controls
        ttk.Label(main_frame, text="Error Value (0-127)").grid(row=0, column=0, sticky=tk.W)
        self.error_scale = ttk.Scale(main_frame, from_=0, to=127, orient=tk.HORIZONTAL, command=self.update_error)
        self.error_scale.grid(row=0, column=1, sticky=(tk.W, tk.E))
        self.error_val = ttk.Label(main_frame, text="0")
        self.error_val.grid(row=0, column=2)

        ttk.Label(main_frame, text="Base Frequency (MIDI Note)").grid(row=1, column=0, sticky=tk.W)
        self.base_scale = ttk.Scale(main_frame, from_=48, to=72, orient=tk.HORIZONTAL, command=self.update_base)
        self.base_scale.grid(row=1, column=1, sticky=(tk.W, tk.E))
        self.base_val = ttk.Label(main_frame, text="60")
        self.base_val.grid(row=1, column=2)
        self.base_scale.set(60)

        # MIDI Output Method
        ttk.Label(main_frame, text="MIDI Mode:").grid(row=2, column=0, sticky=tk.W)
        self.mode_var = tk.StringVar(value="pipe")
        ttk.Radiobutton(main_frame, text="Pipe to Cmd", variable=self.mode_var, value="pipe").grid(row=2, column=1, sticky=tk.W)
        ttk.Radiobutton(main_frame, text="Raw Byte Loopback", variable=self.mode_var, value="raw").grid(row=2, column=2, sticky=tk.W)

        ttk.Label(main_frame, text="MIDI Command:").grid(row=3, column=0, sticky=tk.W)
        # We use a command that can handle raw MIDI bytes.
        self.midi_cmd_var = tk.StringVar(value="timidity -")
        self.midi_cmd_entry = ttk.Entry(main_frame, textvariable=self.midi_cmd_var)
        self.midi_cmd_entry.grid(row=3, column=1, columnspan=2, sticky=(tk.W, tk.E))

        # Connection status and Start/Stop
        self.status_var = tk.StringVar(value="Status: Disconnected")
        ttk.Label(main_frame, textvariable=self.status_var).grid(row=4, column=0, columnspan=3, pady=5)

        self.start_btn = ttk.Button(main_frame, text="Start MIDI", command=self.toggle_midi)
        self.start_btn.grid(row=5, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=6, column=0, columnspan=3, pady=10)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=7, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "1. Run 'make' to build the C++ logic.\n"
            "2. Ensure TiMidity is running.\n"
            "3. If 'timidity -iA' is running in background,\n"
            "   try command: 'aplaymidi -p 128:0 -'\n"
            "   OR: 'timidity -' (launches new instance)\n"
            "4. Click 'Start MIDI'."
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

    def toggle_midi(self):
        if not self.running:
            self.start_midi()
        else:
            self.stop_midi()

    def start_midi(self):
        try:
            # 1. Start MIDI Sink
            if self.mode_var.get() == "pipe":
                self.midi_process = subprocess.Popen(
                    self.midi_cmd_var.get(),
                    shell=True,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE
                )

            # 2. Start Bridge
            self.bridge = subprocess.Popen(
                [self.bridge_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=sys.stderr,
                bufsize=0
            )

            self.running = True
            self.start_btn.config(text="Stop MIDI")
            self.status_var.set("Status: Running")

            # 3. Start forwarding thread
            self.forward_thread = threading.Thread(target=self.forward_midi, daemon=True)
            self.forward_thread.start()

            # Sync initial values
            self.update_error(self.error_scale.get())
            self.update_base(self.base_scale.get())

        except Exception as e:
            messagebox.showerror("Error", f"Failed to start MIDI: {e}")
            self.stop_midi()

    def stop_midi(self):
        self.running = False
        if self.bridge:
            try:
                self.bridge.stdin.write(b"q\n")
                self.bridge.stdin.flush()
            except: pass
            self.bridge.terminate()
        if self.midi_process:
            self.midi_process.terminate()

        self.bridge = None
        self.midi_process = None
        self.start_btn.config(text="Start MIDI")
        self.status_var.set("Status: Disconnected")

    def forward_midi(self):
        try:
            while self.running:
                data = self.bridge.stdout.read(1)
                if not data:
                    break

                if self.mode_var.get() == "pipe" and self.midi_process and self.midi_process.stdin:
                    try:
                        self.midi_process.stdin.write(data)
                        self.midi_process.stdin.flush()
                    except BrokenPipeError:
                        self.status_var.set("Status: Error (Broken Pipe)")
                        self.running = False
                        break
                elif self.mode_var.get() == "raw":
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()

        except Exception as e:
            if self.running:
                print(f"Forwarding error: {e}", file=sys.stderr)

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"e{v}\n".encode())
                self.bridge.stdin.flush()
            except: pass

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"b{v}\n".encode())
                self.bridge.stdin.flush()
            except: pass

    def quit(self):
        self.stop_midi()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MIDIController(root)
    root.protocol("WM_DELETE_WINDOW", app.quit)
    root.mainloop()
