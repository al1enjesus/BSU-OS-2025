# Makefile для lab4
# ------------------------------------------------------------
# Быстрые команды:
#   make        — сборка всех программ (Release, -O2)
#   make debug  — сборка всех программ (Debug, -O0 -g)
#   make clean  — удалить bin/ и объекты
#   make help   — список целей
# ------------------------------------------------------------

# Компилятор и флаги
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pipe
LDFLAGS  ?=
LDLIBS   ?=

# Каталоги
SRC_DIR  := src
BIN_DIR  := bin

# Источники и целевые бинарники
SOURCES  := $(wildcard $(SRC_DIR)/*.cpp)
# Явно перечислим ожидаемые программы для стабильного порядка
TARGETS  := $(BIN_DIR)/A_mem \
            $(BIN_DIR)/B_io_bench \
            $(BIN_DIR)/C_io_monitor \
            $(BIN_DIR)/memory_profiler

# Правило по умолчанию
.PHONY: all
all: $(TARGETS)

# Создаём bin, если его нет
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Общая формула сборки: bin/name из src/name.cpp
$(BIN_DIR)/%: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

# Режим отладки: переопределяем флаги и пересобираем
.PHONY: debug
debug: CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -pipe
debug: clean all

# Удаление результатов сборки
.PHONY: clean
clean:
	@rm -rf $(BIN_DIR) *.o *.obj *.d *.pch

# Справка
.PHONY: help
help:
	@echo "Доступные цели:"
	@echo "  make        — сборка всех программ (Release, -O2)"
	@echo "  make debug  — сборка всех программ (Debug, -O0 -g)"
	@echo "  make clean  — удалить bin/ и промежуточные файлы"
	@echo "  make help   — показать эту справку"

# Явные зависимости, если нужно линкуется с дополнительными библиотеками:
# Пример: добавьте -lrt на старых системах
# $(BIN_DIR)/B_io_bench: LDLIBS += -lrt
