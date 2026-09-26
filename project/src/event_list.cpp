// Окно последних событий — односвязный список.
//
// Зачем список, когда есть std::vector. Затем, что на занятии 1.2 разбирается
// владение: список владеет узлами и обязан их вернуть, а указатель на
// следующий узел — просто способ пройти по цепочке. Вектор всё это скрывает,
// и посмотреть на владение становится негде. С занятия 4.1 список
// действительно заменяется стандартным контейнером, и там же замеряется,
// что от этого изменилось.
//
// Это агрегат: поля открыты, методов нет. Единственное исключение —
// деструктор, и он не роскошь, а требование: см. ниже.
//
// К занятию 2.1 у списка появятся приватные поля, инвариант «size равен числу
// узлов» и методы вместо свободных функций. Разница между «структурой, за
// которой следит вызывающий» и «типом, который следит за собой сам», станет
// видна на своём же коде.

#include <cstddef>

// #include "l1.2/event.h"
// #include "l1.2/event_list.h"
#include "event.h"
#include "event_list.h"

namespace nano_edr {

// Убирает самое старое событие. На пустом списке — ничего не делает
// и не считается ошибкой: «убрать из пустого» это обычный ход событий,
// а не исключительная ситуация.
void ListPopFront(EventList* list) {
    if (list->size == 0)
    {
        return;
    }
    auto head = list->head->next;
    delete list->head;
    list->head = head;
    --list->size;
    if (list->size == 0)
    {
        list->tail = nullptr;
    }
}

// Дописывает копию события в конец.
//
// Если capacity задана и список полон, сначала выбрасывается самое старое
// событие: список — окно последних, а не архив. Значит после вызова
// size <= capacity всегда, и это то, что проверяют тесты.
void ListPushBack(EventList* list, const Event* event) {
    auto node = new EventNode{*event, nullptr};
    if (list->capacity != 0 && (list->size == list->capacity))
    {
        ListPopFront(list);
    }
    if (list->head == nullptr)
    {
        list->head = node;
        list->tail = node;
        ++list->size;
        return;
    }
    
    list->tail->next = node;
    list->tail = node;
    ++list->size;
}

// Освобождает всё. После вызова список пуст и пригоден к использованию снова.
void ListClear(EventList* list) {
    while (list->head != nullptr)
    {
        auto next = list->head->next;
        delete list->head;
        list->head = next;
    }
    list->size = 0;
    list->tail = nullptr;
}

}  // namespace nano_edr

nano_edr::EventList::~EventList() {
    ListClear(this);
}
