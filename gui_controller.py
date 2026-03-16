import tkinter as tk
from tkinter import ttk
import subprocess
import threading
import sys
import os

class MIDIController:
    def __init__(self, root):
        self.root = root
        self.root.title("PWM MIDI Controller")

        # Determine path to bridge
        base_dir = os.path.dirname(os.path.abspath(__file__))
        bridge_path = os.path.join(base_dir, 'tests', 'gui_bridge')

        if not os.path.exists(bridge_path):
            print(f"Error: Bridge binary not found at {bridge_path}. Please run 'make' first.")
            sys.exit(1)

        # Launch the bridge. It outputs raw MIDI bytes to its stdout.
        self.bridge = subprocess.Popen(
            [bridge_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=sys.stderr,
            bufsize=0
        )

        self.setup_ui()

        # Start a thread to forward bridge stdout to this script's stdout
        self.forward_thread = threading.Thread(target=self.forward_midi, daemon=True)
        self.forward_thread.start()

    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

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

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=2, column=0, columnspan=3)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=3, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "This GUI controls the MIDI generation parameters.\n"
            "MIDI bytes are sent to stdout.\n\n"
            "To hear audio on Linux:\n"
            "1. Start TiMidity in background: 'timidity -iA &'\n"
            "2. Run this controller piped to timidity:\n"
            "   python3 gui_controller.py | timidity -iA -"
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

    def forward_midi(self):
        try:
            # We must use the binary buffer for MIDI bytes
            while True:
                data = self.bridge.stdout.read(1)
                if not data:
                    break
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
        except Exception as e:
            # Silently fail on exit
            pass

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        try:
            self.bridge.stdin.write(f"e{v}\n".encode())
            self.bridge.stdin.flush()
        except: pass

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        try:
            self.bridge.stdin.write(f"b{v}\n".encode())
            self.bridge.stdin.flush()
        except: pass

    def quit(self):
        try:
            self.bridge.stdin.write(b"q\n")
            self.bridge.stdin.flush()
        except: pass
        self.bridge.terminate()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MIDIController(root)
    root.protocol("WM_DELETE_WINDOW", app.quit)
    root.mainloop()
