flowchart LR
  App["Process Monitor (user space)"]
  UI["Console UI\n(display & input)"]
  Reader["ProcessReader\n(read /proc/*)"]
  Parser["Parse & Build\nProcessInfo"]
  Manager["ProcessManager\n(syscalls: kill, setpriority)"]
  Kernel[Linux Kernel]
  ProcFS["/proc (virtual FS)"]
  Syscall["System Calls\n(kernel APIs)"]

  App --> UI
  UI -->|request list| Reader
  Reader -->|read files| ProcFS
  ProcFS --> Kernel
  ProcFS -->|raw text| Reader
  Reader --> Parser
  Parser --> UI
  UI -->|user command| Manager
  Manager -->|syscall| Syscall
  Syscall --> Kernel
  Kernel -->|result / errno| Syscall
  Syscall --> Manager
  Manager --> UI
  UI --> App

  subgraph Notes[" "]
    direction LR
    N1["Two data flows: read-only (/proc) and control (syscalls)"]
  end
