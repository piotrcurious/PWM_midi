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

        self.bridge = subprocess.Popen(
            [bridge_path],
            stdin=subprocess.PIPE,
            stdout=sys.stdout,
            stderr=sys.stderr,
            text=True,
            bufsize=1
        )

        self.setup_ui()
        self.running = True
        self.play_loop()

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
        self.base_scale.set(60)
        self.base_val = ttk.Label(main_frame, text="60")
        self.base_val.grid(row=1, column=2)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=2, column=0, columnspan=3)

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        self.bridge.stdin.write(f"e{v}\n")
        self.bridge.stdin.flush()

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        self.bridge.stdin.write(f"b{v}\n")
        self.bridge.stdin.flush()

    def play_loop(self):
        if self.running:
            self.bridge.stdin.write("p\n")
            self.bridge.stdin.flush()
            # The playChordProgression has its own delays (250ms x 2),
            # so we wait a bit less than that to keep it continuous but allow for the delays in C++
            self.root.after(100, self.play_loop)

    def quit(self):
        self.running = False
        self.bridge.stdin.write("q\n")
        self.bridge.stdin.flush()
        self.bridge.terminate()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MIDIController(root)
    root.protocol("WM_DELETE_WINDOW", app.quit)
    root.mainloop()
