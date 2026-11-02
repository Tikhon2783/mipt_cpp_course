#include "event_list.h"

#include "event.h"

namespace nano_edr {

// Убирает самое старое событие. На пустом списке — ничего не делает
// и не считается ошибкой: «убрать из пустого» это обычный ход событий,
// а не исключительная ситуация.
void EventList::PopFront() {
    if (size_ == 0) {
        return;
    }
    auto head = head_->next;
    delete head_;
    head_ = head;
    --size_;
    if (size_ == 0) {
        tail_ = nullptr;
    }
}

// Дописывает копию события в конец.
//
// Если capacity задана и список полон, сначала выбрасывается самое старое
// событие: список — окно последних, а не архив. Значит после вызова
// size <= capacity всегда, и это то, что проверяют тесты.
void EventList::PushBack(const Event& event) {
    if (capacity_ != 0 && size_ == capacity_) {
        PopFront();
    }
    auto node = new EventNode(event);
    if (head_ == nullptr) {
        head_ = node;
    } else {
        tail_->next = node;
    }

    tail_ = node;
    ++size_;
}

// Освобождает всё. После вызова список пуст и пригоден к использованию снова.
void EventList::Clear() {
    while (head_ != nullptr) {
        PopFront();
    }
}

EventList::~EventList() {
    Clear();
}

}  // namespace nano_edr
