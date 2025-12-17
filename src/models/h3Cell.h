#ifndef Q_HEX_WALKER_H3CELL_H
#define Q_HEX_WALKER_H3CELL_H

#include "iH3Data.h"

class H3Cell final : public IH3Data {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(H3Cell)
public:
    explicit H3Cell(QObject *parent = nullptr);
    ~H3Cell() override;
};

#endif  // Q_HEX_WALKER_H3CELL_H