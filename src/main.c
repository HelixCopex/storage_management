#include "ui.h"
#include "csv.h"

int main()
{
    ui_t ui;

    loadCSV(
        "data.csv"
    );

    ui_new(
        SCREEN_MAIN,
        &ui
    );

    uiInit(
        &ui
    );

    ui_draw(
        &ui
    );

    ui_loop(&ui)
    {
        ui_update(&ui);
    }

    ui_free(
        &ui
    );

    return 0;
}