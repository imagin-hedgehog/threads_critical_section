/*mutex заменяем на критическую секцию*/
#include <Windows.h>
#include <conio.h>
#include <stdio.h>

// количество потоков
#define THREADCOUNT 2

// Прототип одного потока
DWORD WINAPI WriteToDataBase(LPVOID);

// объект критической секции для db(базы данных)
// делаем глобальным, чтобы был виден во всех потоках
CRITICAL_SECTION db_cs;
// ГЛОБАЛЬНЫЙ счётчик записей в базу данных
DWORD total_record_count=0;

void main()
{
    // массив обработчиков потоков
    HANDLE aThread[THREADCOUNT];
    // ID потока - нужно при его создании с помощью CreateThread
    DWORD ThreadID;
    int i;  // для перебора потоков

    // Инициализация критической секции c обработкой возможных ошибок
    // 4 - число спин блокироваочных циклов, на однопроцессорных машинах
    // игнорируется и всегда = 0
    // функция возвращает FALSE - если была какая-то ошибка
    // если бы использовали бы обычную функцию InitializeCriticalSection
    // то пришлось бы структурно обрабатывать вход в критическую секцию
    // (try - catch), тк EnterCriticalSection моглы вызывать исключение
    // EXCEPTION_INVALID_HANDLE - меньше кода - лучше читаемость
    if (InitializeCriticalSectionAndSpinCount(&db_cs, 4) == FALSE)
    {   
        // ошибка при инициализации критической секции
        printf("InitializeCS error: %d\n", GetLastError());
    }

    // Создание рабочих потоков
    for(i=0; i < THREADCOUNT; i++)
    {   
        // создаем ОДИН поток
        aThread[i] = CreateThread(
                   NULL, // default sucurrity attributes 
                   0,    // размер стека по умолчанию
                   (LPTHREAD_START_ROUTINE) WriteToDataBase, // функция потока
                   NULL, // т.е. нет аргуметов для функции потока
                   0,    // default creation flags
                   &ThreadID); // записываем ID новоиспечённого потока в ThreadID
        
        // проверяем правильно ли он создался
        if (aThread[i] == NULL)
        {
            // ошибка при создании потока
            printf("CreateThread error: %d\n", GetLastError());
            return;
        }
    }

    // Ожидаем всех потоков до завершения
    WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);


    // Все потоки отработали -> закрываем их по одному
    for( i=0; i < THREADCOUNT; i++ )
        CloseHandle(aThread[i]);

    // здесь мы уже вне цикла - сообщаем что все потоки успешно закрыты
    printf("Threads closed\n");

    // все что можно уже отработала -> нам не нужна критическая секция
    // удалаяем её
    DeleteCriticalSection(&db_cs);
    // сообщаем об этом
    printf("CRITICAL_SECTION deleted\n");

    printf("Press [Enter] to exit: ");
    _getch(); // задержка перед выходом


    // валим из этой функции к лучшей жизни
    return;
}


/*Функция одного потока*/
DWORD WINAPI WriteToDataBase( LPVOID lpParam )
{ 
  // ЛОКАЛЬНЫЙ счётчик записей в базу данных
  // то есть только для этого потока
  DWORD dwCount=0;

  // будем пытаться записывать в базу 10 раз
  // причём суммарное число записей в базу данных равно
  // произведению числа потоков на 10 - записи выполняемые одним потоком
  // то есть THREADCOUNT * 10
  // в нашем случае THREADCOUNT * 10 = 2 * 10 = 20
  while (dwCount < 10)
  {
    // входим в критическую секцию
    EnterCriticalSection(&db_cs);

    // мы внутри - то есть получили МОНОПОЛЬНЫЙ доступ к нашей 'базе'
    // '' - тк настоящей базы нет, ведь мы просто 
    // выводим сообщения в стандартный вывод
    // - что-то записываем в 'базу данных'

    /*Манипуляции c 'базой данных'*/
    // новая запись в базу - увеличиваем ГЛОБАЛЬНЫЙ счётчки записей
    total_record_count++;
    // красиво выводим номер записи в базу и тд
    printf("[%02d]Thread %d writing to database...\n", total_record_count, GetCurrentThreadId());
    // сколько раз мы записывали в базу в ЭТОМ ПОТОКЕ
    // пременная - используется ТОЛЬКО в этом потоке
    dwCount++;

    // после каждой (ОДНОЙ) записи даём возможность другим потокам
    // взаимодействовать с базой данных
    // то есть выходим из критической секции
    LeaveCriticalSection(&db_cs);
  }

  // все прошло успешно поток отработал
  return TRUE; 
}
