#pragma once

enum class OrderType
{
    Day,
    GoodTillCanceled,
    GoodTillDate,
    FillAndKill,
    FillOrKill,
    MinimumQuantity,
    DisplayQuantity,
};