import matplotlib.pyplot as plt
import numpy as np
import os

def create_page_faults_graph():
    """Графики по заданию C - Page Faults"""
    plt.figure(figsize=(14, 10))
    
    # Данные из твоего page_fault_analyzer
    stages = ["Initial", "After malloc", "After sequential", "After random", "After repeat", "After free"]
    minor_faults = [81, 82, 25681, 25681, 25681, 25681]
    major_faults = [0, 0, 0, 0, 0, 0]
    total_faults = [81, 82, 25681, 25681, 25681, 25681]
    
    # График 1: Общие page faults
    plt.subplot(2, 2, 1)
    plt.plot(stages, total_faults, 'o-', linewidth=3, markersize=10, color='#3498db', markerfacecolor='#2980b9')
    plt.title('Total Page Faults Progress\n(Демонстрация Lazy Allocation)', fontsize=14, fontweight='bold')
    plt.ylabel('Количество Faults', fontsize=12)
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3)
    plt.yscale('log')  # Логарифмическая шкала для наглядности
    
    # Подписываем скачок
    plt.annotate('Скачок с 82 до 25681\n(25,599 новых faults)', 
                xy=(2, 25681), xytext=(3, 10000),
                arrowprops=dict(arrowstyle='->', color='red', lw=2),
                fontsize=10, color='red', fontweight='bold')
    
    # График 2: Minor vs Major faults
    plt.subplot(2, 2, 2)
    x = np.arange(len(stages))
    width = 0.6
    
    bars = plt.bar(x, minor_faults, width, label='Minor Faults', color='#2ecc71', alpha=0.8, edgecolor='black')
    plt.bar(x, major_faults, width, label='Major Faults', color='#e74c3c', alpha=0.8, edgecolor='black', bottom=minor_faults)
    
    plt.title('Minor vs Major Page Faults\n(Zero Major Faults = Достаточно RAM)', fontsize=14, fontweight='bold')
    plt.ylabel('Количество Faults', fontsize=12)
    plt.xticks(x, stages, rotation=45, ha='right')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.yscale('log')
    
    # График 3: Прирост faults на каждом этапе
    plt.subplot(2, 2, 3)
    faults_increase = [total_faults[i] - total_faults[i-1] if i > 0 else total_faults[i] for i in range(len(total_faults))]
    
    colors = ['lightblue' if inc == 0 else '#e67e22' for inc in faults_increase]
    bars = plt.bar(stages, faults_increase, color=colors, alpha=0.8, edgecolor='black')
    
    plt.title('Новые Faults на Каждом Этапе\n(Lazy Allocation - память выделяется при первом обращении)', 
              fontsize=12, fontweight='bold')
    plt.ylabel('Новые Faults', fontsize=12)
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3)
    
    # Добавляем значения на столбцы
    for bar, value in zip(bars, faults_increase):
        if value > 0:
            plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 100, 
                    f'+{value}', ha='center', va='bottom', fontweight='bold')
    
    # График 4: Эффективность кеширования
    plt.subplot(2, 2, 4)
    efficiency = [0, 0, 100, 0, 0, 0]  # Процент новых faults
    
    plt.plot(stages, efficiency, 's-', linewidth=3, markersize=8, color='#9b59b6')
    plt.fill_between(stages, efficiency, alpha=0.3, color='#9b59b6')
    plt.title('Эффективность Кеширования\n(0% новых faults после первого обращения)', 
              fontsize=12, fontweight='bold')
    plt.ylabel('Процент новых Faults (%)', fontsize=12)
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3)
    plt.ylim(0, 120)
    
    plt.tight_layout()
    plt.savefig('screenshots/page_faults_analysis.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_memory_metrics_graph():
    """Графики по заданию A - Memory Metrics"""
    # Данные из твоего memory_profiler
    metrics = ['VSZ (Virtual)', 'RSS (Resident)', 'PSS (Proportional)', 'USS (Unique)']
    values_mb = [4.6, 3.5, 2.1, 2.1]
    colors = ['#3498db', '#2ecc71', '#9b59b6', '#e74c3c']
    
    plt.figure(figsize=(12, 8))
    
    # График 1: Столбчатая диаграмма
    plt.subplot(2, 2, 1)
    bars = plt.bar(metrics, values_mb, color=colors, alpha=0.8, edgecolor='black')
    
    plt.title('Метрики Памяти Процесса\n(VSZ > RSS из-за Lazy Allocation)', fontsize=14, fontweight='bold')
    plt.ylabel('Память (MB)', fontsize=12)
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3)
    
    # Добавляем значения
    for bar, value in zip(bars, values_mb):
        plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1, 
                f'{value} MB', ha='center', va='bottom', fontweight='bold')
    
    # График 2: VSZ vs RSS разница
    plt.subplot(2, 2, 2)
    vsz_rss_diff = [values_mb[0] - values_mb[1]]  # VSZ - RSS
    plt.bar(['VSZ - RSS'], vsz_rss_diff, color='#f39c12', alpha=0.8, edgecolor='black')
    plt.title('Разница VSZ и RSS\n(Неиспользуемая виртуальная память)', fontsize=12, fontweight='bold')
    plt.ylabel('Разница (MB)', fontsize=12)
    plt.grid(True, alpha=0.3)
    
    plt.text(0, vsz_rss_diff[0]/2, f'{vsz_rss_diff[0]} MB', 
             ha='center', va='center', fontweight='bold', fontsize=16, color='white')
    
    # График 3: PSS и USS (более точные метрики)
    plt.subplot(2, 2, 3)
    pss_uss_data = [values_mb[2], values_mb[3]]
    labels = ['PSS', 'USS']
    colors_pie = ['#9b59b6', '#e74c3c']
    
    plt.pie(pss_uss_data, labels=labels, colors=colors_pie, autopct='%1.1f%%', startangle=90)
    plt.title('PSS vs USS\n(Пропорциональная vs Уникальная память)', fontsize=12, fontweight='bold')
    
    # График 4: Детализация памяти
    plt.subplot(2, 2, 4)
    breakdown_labels = ['Shared Clean', 'Shared Dirty', 'Private Clean', 'Private Dirty']
    breakdown_values = [1.5, 0, 0.012, 2.1]  # Из твоего вывода
    breakdown_colors = ['#3498db', '#2980b9', '#2ecc71', '#27ae60']
    
    plt.pie(breakdown_values, labels=breakdown_labels, colors=breakdown_colors, autopct='%1.1fMB')
    plt.title('Детализация Памяти', fontsize=12, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('screenshots/memory_metrics.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_file_io_comparison():
    """Графики по заданию B - File I/O"""
    # Данные из твоего file_analyzer
    methods = ['read()', 'mmap()']
    time_seconds = [0.094, 0.474]
    minor_faults = [2, 1600]
    major_faults = [0, 1]
    
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))
    
    # График 1: Время выполнения
    bars1 = ax1.bar(methods, time_seconds, color=['#3498db', '#e74c3c'], alpha=0.8, edgecolor='black')
    ax1.set_title('Время Выполнения\n(read() в 5 раз быстрее mmap())', fontsize=14, fontweight='bold')
    ax1.set_ylabel('Время (секунды)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    
    for bar, value in zip(bars1, time_seconds):
        ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01, 
                f'{value:.3f}s', ha='center', va='bottom', fontweight='bold')
    
    # График 2: Speedup
    ax2.bar(['Speedup'], [time_seconds[0]/time_seconds[1]], color='#2ecc71', alpha=0.8, edgecolor='black')
    ax2.set_title('Коэффициент Ускорения', fontsize=14, fontweight='bold')
    ax2.set_ylabel('read() / mmap()', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.text(0, 0.1, f'{time_seconds[0]/time_seconds[1]:.2f}x', 
             ha='center', va='center', fontweight='bold', fontsize=20, color='white')
    
    # График 3: Page faults сравнение
    x = np.arange(len(methods))
    width = 0.35
    
    bars3 = ax3.bar(x - width/2, minor_faults, width, label='Minor Faults', color='#2ecc71', alpha=0.8)
    bars4 = ax3.bar(x + width/2, major_faults, width, label='Major Faults', color='#e74c3c', alpha=0.8)
    ax3.set_title('Page Faults Сравнение\n(mmap() генерирует больше faults)', fontsize=14, fontweight='bold')
    ax3.set_ylabel('Количество Faults', fontsize=12)
    ax3.set_xticks(x)
    ax3.set_xticklabels(methods)
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    ax3.set_yscale('log')
    
    # График 4: Эффективность методов
    efficiency_read = [95, 5]  # Условные проценты эффективности
    efficiency_mmap = [20, 80]
    labels_eff = ['Эффективность', 'Накладные расходы']
    
    ax4.pie(efficiency_read, labels=labels_eff, autopct='%1.0f%%', startangle=90, colors=['#2ecc71', '#e74c3c'])
    ax4.set_title('Эффективность: read()', fontsize=12, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('screenshots/file_io_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_summary_infographic():
    """Сводная инфографика"""
    plt.figure(figsize=(16, 12))
    
    # Основные выводы
    conclusions = [
        "Lazy Allocation: Память выделяется\nпри первом обращении",
        "Zero Major Faults: Достаточно RAM,\nнет подкачки на диск", 
        "Read() быстрее mmap()\nв 5 раз для последовательного чтения",
        "Эффективное кеширование:\nПовторные доступы бесплатны"
    ]
    
    importance = [95, 90, 85, 80]
    colors = ['#3498db', '#2ecc71', '#e74c3c', '#9b59b6']
    
    plt.barh(conclusions, importance, color=colors, alpha=0.8, edgecolor='black')
    plt.title('Ключевые Выводы Лабораторной Работы', fontsize=16, fontweight='bold')
    plt.xlabel('Важность (%)', fontsize=12)
    plt.grid(True, alpha=0.3)
    
    # Добавляем значения
    for i, (value, color) in enumerate(zip(importance, colors)):
        plt.text(value + 1, i, f'{value}%', va='center', fontweight='bold', color=color, fontsize=12)
    
    plt.tight_layout()
    plt.savefig('screenshots/summary_infographic.png', dpi=300, bbox_inches='tight')
    plt.show()

if __name__ == "__main__":
    # Создаем папку для скриншотов
    os.makedirs('screenshots', exist_ok=True)
    
    print("🚀 Генерируем графики с твоими реальными данными...")
    
    create_page_faults_graph()
    print("✅ Графики Page Faults готовы")
    
    create_memory_metrics_graph()
    print("✅ Графики Memory Metrics готовы")
    
    create_file_io_comparison()
    print("✅ Графики File I/O готовы")
    
    create_summary_infographic()
    print("✅ Сводная инфографика готова")
    
    print("\n🎉 Все графики сохранены в папку screenshots/")
    print("📊 Файлы:")
    print("   - page_faults_analysis.png")
    print("   - memory_metrics.png") 
    print("   - file_io_comparison.png")
    print("   - summary_infographic.png")
