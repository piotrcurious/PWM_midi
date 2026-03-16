import tkinter as tk
from tkinter import ttk, messagebox
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
        self.bridge_path = os.path.join(base_dir, 'tests', 'gui_bridge')

        if not os.path.exists(self.bridge_path):
            messagebox.showerror("Error", f"Bridge binary not found at {self.bridge_path}.\nPlease run 'make' first.")
            sys.exit(1)

        self.bridge = None
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

        # Connection status and Start/Stop
        self.status_var = tk.StringVar(value="Status: Disconnected")
        ttk.Label(main_frame, textvariable=self.status_var).grid(row=2, column=0, columnspan=3, pady=5)

        self.start_btn = ttk.Button(main_frame, text="Start MIDI", command=self.toggle_midi)
        self.start_btn.grid(row=3, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=4, column=0, columnspan=3, pady=10)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=5, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "1. Run 'make' to build the ALSA-enabled logic.\n"
            "2. Ensure TiMidity is running ('timidity -iA').\n"
            "3. Click 'Start MIDI'. The bridge will connect to ALSA.\n"
            "4. If no sound, use 'aconnect -l' to find ports and\n"
            "   'aconnect \"PWM MIDI Bridge\" TiMidity' manually."
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

    def toggle_midi(self):
        if not self.running:
            self.start_midi()
        else:
            self.stop_midi()

    def start_midi(self):
        try:
            # Start Bridge
            self.bridge = subprocess.Popen(
                [self.bridge_path],
                stdin=subprocess.PIPE,
                stdout=sys.stdout,
                stderr=sys.stderr,
                text=True,
                bufsize=1
            )

            self.running = True
            self.start_btn.config(text="Stop MIDI")
            self.status_var.set("Status: Running (Direct ALSA)")

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
                self.bridge.stdin.write("q\n")
                self.bridge.stdin.flush()
            except: pass
            self.bridge.terminate()

        self.bridge = None
        self.start_btn.config(text="Start MIDI")
        self.status_var.set("Status: Disconnected")

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"e{v}\n")
                self.bridge.stdin.flush()
            except: pass

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"b{v}\n")
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
