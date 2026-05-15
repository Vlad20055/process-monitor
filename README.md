# Process Monitor

Утилита для просмотра и управления процессами Linux.

## Сборка

cmake --build build -- -j2
```


## Запуск и опции командной строки

Синтаксис:
./build/process_monitor [--once] [--watch N] [--name-filter STR] [--min-cpu SECS] [--user NAME]
```

- `--once` — распечатать список процессов один раз и выйти.
- `--watch N` — включить авто-обновление каждые `N` секунд при старте.
- `--name-filter STR` — показывать только процессы, у которых имя или `cmdline` содержит `STR` (поиск нечувствителен к регистру).
- `--min-cpu SECS` — показывать только процессы с суммарным CPU (в секундах) >= `SECS`.
- `--user NAME` — показывать только процессы, принадлежащие пользователю `NAME`.

Примеры:

- Печать один раз:
  `./build/process_monitor --once`
- Авто-обновление каждую секунду:
  `./build/process_monitor --watch 1`
- Авто-обновление и фильтр по имени `ssh`:
  `./build/process_monitor --watch 2 --name-filter ssh`
- Только процессы пользователя `root` с CPU >= 1.5s:
  `./build/process_monitor --user root --min-cpu 1.5 --watch 2`

## Интерактивные команды (внутри программы)

После запуска без `--once` программа входит в интерактивный режим и поддерживает команды:

- `help` — показать помощь.
- `refresh` или `r` — немедленно обновить список.
- `watch <sec>` — включить авто-обновление с указанным интервалом (в секундах).
- `resume <pid>` — отправить SIGCONT.
- `nice <pid> <value>` — сменить значение nice (диапазон `[-20, 19]`).
- `quit` или `exit` — выход.

## Tests
  - `scripts/test_perf.sh` — выполняет `build/process_monitor --once` N раз и выводит среднее время выполнения.
  - `scripts/test_terminate.sh` — создаёт фоновую команду `sleep`, запускает `process_monitor`, посылает через stdin `kill <pid>` и проверяет, что процесс завершился.

### Запуск через make

```sh
make test-perf         # производительный тест
make test-terminate    # корректность завершения процесса
make test              # запускает оба теста
```

Параметры можно переопределить для `test-perf`, например:

```sh
make test-perf TEST_RUNS=10 EXPECT_MAX=1.5
```
