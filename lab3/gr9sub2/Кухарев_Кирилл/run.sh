#!/bin/bash

set -e  

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' 

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_section() {
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
}


check_build_system() {
    if [ ! -f "Makefile" ]; then
        print_error "Makefile не найден в текущей директории"
        exit 1
    fi
}

build_projects() {
    print_section "СБОРКА ПРОЕКТОВ"
    print_info "Компиляция всех программ..."
    
    if make all; then
        print_success "Все программы успешно скомпилированы"
    else
        print_error "Ошибка компиляции"
        exit 1
    fi
}

test_race_unsync() {
    print_section "ТЕСТ 1: ГОНКА ДАННЫХ (UNSYNC)"
    print_info "Демонстрация проблемы гонки данных..."
    
    if ./thread_race 4 1000000 unsync; then
        print_warning "Тест завершен (ожидается расхождение данных)"
    else
        print_error "Тест завершился с ошибкой"
    fi
}

test_race_mutex() {
    print_section "ТЕСТ 2: МЬЮТЕКС (MUTEX)"
    print_info "Демонстрация корректной синхронизации..."
    
    if ./thread_race 4 1000000 mutex; then
        print_success "Тест завершен (данные корректны)"
    else
        print_error "Тест завершился с ошибкой"
    fi
}

test_race_atomic() {
    print_section "ТЕСТ 3: АТОМИКИ (ATOMIC)"
    print_info "Демонстрация атомарных операций..."
    
    if ./thread_race 4 1000000 atomic; then
        print_success "Тест завершен (данные корректны)"
    else
        print_error "Тест завершился с ошибкой"
    fi
}

test_prodcons() {
    print_section "ТЕСТ 4: PRODUCER-CONSUMER"
    print_info "Тестирование паттерна производитель-потребитель..."
    
    if ./prodcons -P 2 -C 2 -N 100000 -B 64; then
        print_success "Тест завершен"
    else
        print_error "Тест завершился с ошибкой"
    fi
}
test_performance() {
    print_section "ТЕСТ 5: АНАЛИЗ ПРОИЗВОДИТЕЛЬНОСТИ"
    
    echo -e "\n${YELLOW}Сравнение производительности при разном количестве потоков:${NC}"
    
    print_info "2 потока, unsync:"
    ./thread_race 2 1000000 unsync
    
    print_info "2 потока, mutex:"
    ./thread_race 2 1000000 mutex
    
    print_info "8 потоков, unsync:"
    ./thread_race 8 1000000 unsync
    
    print_info "8 потоков, mutex:"
    ./thread_race 8 1000000 mutex
}

test_prodcons_variants() {
    print_section "ТЕСТ 6: РАЗЛИЧНЫЕ КОНФИГУРАЦИИ PRODUCER-CONSUMER"
    
    print_info "Конфигурация 1: 1 производитель, 1 потребитель"
    ./prodcons -P 1 -C 1 -N 50000 -B 32
    
    print_info "Конфигурация 2: 4 производителя, 2 потребителя"
    ./prodcons -P 4 -C 2 -N 200000 -B 128
    
    print_info "Конфигурация 3: 2 производителя, 4 потребителя"
    ./prodcons -P 2 -C 4 -N 200000 -B 64
}

analyze_threads() {
    print_section "АНАЛИЗ ПОТОКОВ В СИСТЕМЕ"
    
    local PID=$$
    print_info "PID текущего процесса: $PID"
    
    echo -e "\n${YELLOW}Список потоков процесса:${NC}"
    if ps -p $PID > /dev/null 2>&1; then
        ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm 2>/dev/null || echo "Не удалось получить информацию о потоках"
    else
        echo "Процесс уже завершился"
    fi
    
    echo -e "\n${YELLOW}Количество потоков:${NC}"
    if [ -f "/proc/$PID/status" ]; then
        cat /proc/$PID/status | grep Threads 2>/dev/null || echo "Не удалось прочитать статус"
    else
        echo "Файл статуса процесса недоступен"
    fi
    
    echo -e "\n${YELLOW}Директория задач:${NC}"
    if [ -d "/proc/$PID/task" ]; then
        ls -l /proc/$PID/task 2>/dev/null | head -10 || echo "Не удалось прочитать директорию задач"
    else
        echo "Директория задач недоступна"
    fi
}


main() {
    print_section "ЛАБОРАТОРНАЯ РАБОТА 3: ПОТОКИ, СИНХРОНИЗАЦИЯ И ГОНКИ ДАННЫХ"
    print_info "Начало выполнения тестов..."
    echo ""
    
    check_build_system
    build_projects
    
    test_race_unsync
    test_race_mutex
    test_race_atomic
    test_prodcons
    
    test_performance
    test_prodcons_variants
    
    analyze_threads
        
    print_section "ТЕСТИРОВАНИЕ ЗАВЕРШЕНО"
    print_success "Все тесты выполнены успешно!"
    echo ""
    print_warning "Обратите внимание на расхождение данных в режиме 'unsync' - это демонстрация гонки данных"
    print_success "Режимы 'mutex' и 'atomic' должны показывать корректные результаты"
}

case "${1:-}" in
    "help"|"-h"|"--help")
        echo "Использование: $0 [команда]"
        echo ""
        echo "Команды:"
        echo "  help     - показать эту справку"
        echo "  fast     - быстрый прогон (только основные тесты)"
        echo "  full     - полный прогон всех тестов (по умолчанию)"
        echo "  build    - только сборка проектов"
        echo ""
        exit 0
        ;;
    "fast")
        print_warning "Запуск быстрого тестирования..."
        check_build_system
        build_projects
        test_race_unsync
        test_race_mutex
        test_race_atomic
        test_prodcons
        ;;
    "build")
        check_build_system
        build_projects
        ;;
    *)
        main
        ;;
esac
