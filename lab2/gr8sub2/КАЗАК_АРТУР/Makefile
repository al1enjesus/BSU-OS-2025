# Makefile (Python-only)
# Запуск Python-утилит без сборки C

.PHONY: all clean run_fork_py run_ptree_py lint

all:
	@echo "Nothing to build (Python-only). Use: make run_fork_py / run_ptree_py"

run_fork_py:
	python3 src/fork_example.py

run_ptree_py:
	python3 src/ptree.py

lint:
	# Лёгкая проверка стиля, не фейлит сборку если pyflakes не установлен
	@if command -v pyflakes >/dev/null 2>&1; then pyflakes src || true; else echo "pyflakes not found (optional)"; fi

clean:
	rm -f *.log
