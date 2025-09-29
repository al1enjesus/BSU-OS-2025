.PHONY: all clean run test

all: fork_example ptree

fork_example:
	@echo "Запуск программы создания процессов..."
	python3 src/fork_example.py

ptree:
	@echo "Запуск утилиты ptree..."
	python3 src/ptree.py

run: all

clean:
	@echo "Очистка завершена"

test: all
	@echo "Тестирование завершено"
