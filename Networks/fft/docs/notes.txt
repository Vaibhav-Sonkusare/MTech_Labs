
# Project Folder Structure

fast-udp-transfer/
│
├── src/
│   ├── sender.c
│   ├── receiver.c
│   ├── protocol.c
│   ├── buffer.c
│   ├── net.c
│   ├── timer.c
│   ├── log.c
│   ├── util.c
│   └── main.c        (optional; sometimes sender/receiver are entry points)
│
├── include/
│   ├── protocol.h
│   ├── buffer.h
│   ├── net.h
│   ├── timer.h
│   ├── log.h
│   └── util.h
│
├── tests/
│   ├── test_protocol.c
│   └── test_buffer.c
│
├── fsm/
│   ├── sender.dot
│   ├── sender.svg
│   ├── receiver.dot
│   └── receiver.svg
│
├── scripts/
│   ├── run_tests.sh
│   ├── run_garbler.sh
│   └── plot_results.py
│
├── data/
│   ├── input/       (files to send)
│   └── output/      (received files)
│
├── docs/
│   ├── design.md
│   ├── protocol.md
│   └── report.pdf   (final report)
│
├── Makefile
├── README.md
└── Term Project-1-Fast File Xfer over UDP.pdf


# Random data files to send genereated using the following command:
> tr -dc "A-Za-z 0-9" < /dev/urandom | fold -w100|head -n 100000 > bigfile.txt

Here, modifing the integer after -n in head command, changes the number of lines in the output file, thereby acheaving the effect of changing test file size