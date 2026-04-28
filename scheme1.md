flowchart TD
    Start([Start]) --> Init[Init components\nread config, set watch interval]
    Init --> Fetch0[Initial fetch:\nProcessReader::list_processes]
    Fetch0 --> Print0[Print header + table\nConsoleUI::refresh_and_print]
    Print0 --> Loop[Main loop]

    Loop --> Wait[Wait: poll with timeout\nor blocking read if watch off]
    Wait --> InputCheck{Event: stdin ready\nor timeout?}

    InputCheck -->|timeout & watch| OnTimeout[Timeout & watch enabled]
    OnTimeout --> Fetch[Fetch updated list]
    Fetch --> Print[Print updated table]
    Print --> Loop

    InputCheck -->|stdin ready| OnInput[Read input line]
    OnInput --> Parse[Parse command\nConsoleUI::handle_command]
    Parse --> IsExit{Is exit command?}

    IsExit -->|yes| End([Exit / Cleanup])
    IsExit -->|no| ManagementCmd{Is control command?\nkill/suspend/resume/nice}

    ManagementCmd -->|yes| HandleMgmt[Call ProcessManager\nupdate status_message]
    HandleMgmt --> UpdateAfterCmd[Refresh display\nshow status_message]
    UpdateAfterCmd --> Loop

    ManagementCmd -->|no| HandleOther[Handle: help/refresh/watch/stop/invalid]
    HandleOther --> UpdateAfterCmd
