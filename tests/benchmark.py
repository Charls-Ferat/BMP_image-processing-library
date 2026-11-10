import csv
import os
import platform
import statistics
import subprocess
import time
from pathlib import Path
from datetime import datetime

# Configuration
REPETITIONS = 10
WARMUP_RUNS = 1

ROOT = Path(__file__).resolve().parent.parent
RESULTS_DIR = ROOT / "results"

# Select executable according to operating system
if platform.system() == "Windows":
    PROGRAM = ROOT / "build" / "image_processor.exe"
else:
    PROGRAM = ROOT / "build" / "image_processor"


def run_program():
    #Execute the C program and return its execution time.
    start = time.perf_counter()

    result = subprocess.run(
        [str(PROGRAM)],
        cwd=ROOT,
        capture_output=True,
        text=False
    )

    """
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
        raise RuntimeError("Program execution failed")
    """

    end = time.perf_counter()

    if result.returncode != 0:
        raise RuntimeError(
            f"Program failed with exit code {result.returncode}:\n"
            f"{result.stderr}"
        )

    return end - start


def main():

    if not PROGRAM.is_file():
        print(f"Executable not found: {PROGRAM}")
        return

    RESULTS_DIR.mkdir(exist_ok=True)

    print("--IMAGE PROCESSING BENCHMARK--")

    print(f"Operating system: {platform.platform()}")
    print(f"Processor: {platform.processor()}")
    print(f"Python version: {platform.python_version()}")
    print(f"Repetitions: {REPETITIONS}")
    print(f"Warmup runs: {WARMUP_RUNS}")

    # Warmup (not included in measurements)
    for _ in range(WARMUP_RUNS):
        run_program()

    # Actual benchmark
    times = []

    print("\nStarting benchmark...\n")

    for i in range(REPETITIONS):

        elapsed = run_program()
        times.append(elapsed)

        print(
            f"Run {i + 1:02d}: "
            f"{elapsed:.6f} seconds"
        )

    # Statistics
    average = statistics.mean(times)
    minimum = min(times)
    maximum = max(times)
    std_dev = statistics.stdev(times) if len(times) > 1 else 0

    print("--BENCHMARK RESULTS--")
    print(f"Average:          {average:.6f} seconds")
    print(f"Minimum:          {minimum:.6f} seconds")
    print(f"Maximum:          {maximum:.6f} seconds")
    print(f"Std. deviation:   {std_dev:.6f} seconds")

    # Save results
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = RESULTS_DIR / f"benchmark_{timestamp}.csv"

    with open(filename, "w", newline="") as file:

        writer = csv.writer(file)
        writer.writerow(["Run", "Execution_Time_Seconds"])

        for i, elapsed in enumerate(times, start=1):
            writer.writerow([i, elapsed])

        writer.writerow([])
        writer.writerow(["Average", average])
        writer.writerow(["Minimum", minimum])
        writer.writerow(["Maximum", maximum])
        writer.writerow(["Standard_Deviation", std_dev])

    print(f"\nResults saved to: {filename}")


if __name__ == "__main__":
    main()
