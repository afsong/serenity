/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 * Copyright (c) 2021, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/Object.h>
#include <LibJS/Runtime/Temporal/AbstractOperations.h>
#include <LibJS/Runtime/Temporal/Calendar.h>
#include <LibJS/Runtime/Temporal/PlainTime.h>
#include <LibJS/Runtime/Temporal/PlainTimeConstructor.h>

namespace JS::Temporal {

// 4 Temporal.PlainTime Objects, https://tc39.es/proposal-temporal/#sec-temporal-plaintime-objects
PlainTime::PlainTime(u8 iso_hour, u8 iso_minute, u8 iso_second, u16 iso_millisecond, u16 iso_microsecond, u16 iso_nanosecond, Calendar& calendar, Object& prototype)
    : Object(prototype)
    , m_iso_hour(iso_hour)
    , m_iso_minute(iso_minute)
    , m_iso_second(iso_second)
    , m_iso_millisecond(iso_millisecond)
    , m_iso_microsecond(iso_microsecond)
    , m_iso_nanosecond(iso_nanosecond)
    , m_calendar(calendar)
{
}

void PlainTime::visit_edges(Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(&m_calendar);
}

// 4.5.4 RegulateTime ( hour, minute, second, millisecond, microsecond, nanosecond, overflow ), https://tc39.es/proposal-temporal/#sec-temporal-regulatetime
Optional<TemporalTime> regulate_time(GlobalObject& global_object, double hour, double minute, double second, double millisecond, double microsecond, double nanosecond, StringView overflow)
{
    auto& vm = global_object.vm();

    // 1. Assert: hour, minute, second, millisecond, microsecond and nanosecond are integers.
    // NOTE: As the spec is currently written this assertion can fail, these are either integers _or_ infinity.
    //       See https://github.com/tc39/proposal-temporal/issues/1672.

    // 2. Assert: overflow is either "constrain" or "reject".
    // NOTE: Asserted by the VERIFY_NOT_REACHED at the end

    // 3. If overflow is "constrain", then
    if (overflow == "constrain"sv) {
        // a. Return ! ConstrainTime(hour, minute, second, millisecond, microsecond, nanosecond).
        return constrain_time(hour, minute, second, millisecond, microsecond, nanosecond);
    }

    // 4. If overflow is "reject", then
    if (overflow == "reject"sv) {
        // a. If ! IsValidTime(hour, minute, second, millisecond, microsecond, nanosecond) is false, throw a RangeError exception.
        if (!is_valid_time(hour, minute, second, millisecond, microsecond, nanosecond)) {
            vm.throw_exception<RangeError>(global_object, ErrorType::TemporalInvalidPlainTime);
            return {};
        }

        // b. Return the Record { [[Hour]]: hour, [[Minute]]: minute, [[Second]]: second, [[Millisecond]]: millisecond, [[Microsecond]]: microsecond, [[Nanosecond]]: nanosecond }.
        return TemporalTime { .hour = hour, .minute = minute, .second = second, .millisecond = millisecond, .microsecond = microsecond, .nanosecond = nanosecond };
    }

    VERIFY_NOT_REACHED();
}

// 4.5.5 IsValidTime ( hour, minute, second, millisecond, microsecond, nanosecond ), https://tc39.es/proposal-temporal/#sec-temporal-isvalidtime
bool is_valid_time(double hour, double minute, double second, double millisecond, double microsecond, double nanosecond)
{
    // 1. Assert: hour, minute, second, millisecond, microsecond, and nanosecond are integers.

    // 2. If hour < 0 or hour > 23, then
    if (hour > 23) {
        // a. Return false.
        return false;
    }

    // 3. If minute < 0 or minute > 59, then
    if (minute > 59) {
        // a. Return false.
        return false;
    }

    // 4. If second < 0 or second > 59, then
    if (second > 59) {
        // a. Return false.
        return false;
    }

    // 5. If millisecond < 0 or millisecond > 999, then
    if (millisecond > 999) {
        // a. Return false.
        return false;
    }

    // 6. If microsecond < 0 or microsecond > 999, then
    if (microsecond > 999) {
        // a. Return false.
        return false;
    }

    // 7. If nanosecond < 0 or nanosecond > 999, then
    if (nanosecond > 999) {
        // a. Return false.
        return false;
    }

    // 8. Return true.
    return true;
}

// 4.5.6 BalanceTime ( hour, minute, second, millisecond, microsecond, nanosecond ), https://tc39.es/proposal-temporal/#sec-temporal-balancetime
DaysAndTime balance_time(i64 hour, i64 minute, i64 second, i64 millisecond, i64 microsecond, i64 nanosecond)
{
    // 1. Assert: hour, minute, second, millisecond, microsecond, and nanosecond are integers.

    // 2. Set microsecond to microsecond + floor(nanosecond / 1000).
    microsecond += nanosecond / 1000;

    // 3. Set nanosecond to nanosecond modulo 1000.
    nanosecond %= 1000;

    // 4. Set millisecond to millisecond + floor(microsecond / 1000).
    millisecond += microsecond / 1000;

    // 5. Set microsecond to microsecond modulo 1000.
    microsecond %= 1000;

    // 6. Set second to second + floor(millisecond / 1000).
    second += millisecond / 1000;

    // 7. Set millisecond to millisecond modulo 1000.
    millisecond %= 1000;

    // 8. Set minute to minute + floor(second / 60).
    minute += second / 60;

    // 9. Set second to second modulo 60.
    second %= 60;

    // 10. Set hour to hour + floor(minute / 60).
    hour += minute / 60;

    // 11. Set minute to minute modulo 60.
    minute %= 60;

    // 12. Let days be floor(hour / 24).
    u8 days = hour / 24;

    // 13. Set hour to hour modulo 24.
    hour %= 24;

    // 14. Return the Record { [[Days]]: days, [[Hour]]: hour, [[Minute]]: minute, [[Second]]: second, [[Millisecond]]: millisecond, [[Microsecond]]: microsecond, [[Nanosecond]]: nanosecond }.
    return DaysAndTime {
        .days = static_cast<i32>(days),
        .hour = static_cast<u8>(hour),
        .minute = static_cast<u8>(minute),
        .second = static_cast<u8>(second),
        .millisecond = static_cast<u16>(millisecond),
        .microsecond = static_cast<u16>(microsecond),
        .nanosecond = static_cast<u16>(nanosecond),
    };
}

// 4.5.7 ConstrainTime ( hour, minute, second, millisecond, microsecond, nanosecond ), https://tc39.es/proposal-temporal/#sec-temporal-constraintime
TemporalTime constrain_time(double hour, double minute, double second, double millisecond, double microsecond, double nanosecond)
{
    // 1. Assert: hour, minute, second, millisecond, microsecond, and nanosecond are integers.

    // 2. Set hour to ! ConstrainToRange(hour, 0, 23).
    hour = constrain_to_range(hour, 0, 23);

    // 3. Set minute to ! ConstrainToRange(minute, 0, 59).
    minute = constrain_to_range(minute, 0, 59);

    // 4. Set second to ! ConstrainToRange(second, 0, 59).
    second = constrain_to_range(second, 0, 59);

    // 5. Set millisecond to ! ConstrainToRange(millisecond, 0, 999).
    millisecond = constrain_to_range(millisecond, 0, 999);

    // 6. Set microsecond to ! ConstrainToRange(microsecond, 0, 999).
    microsecond = constrain_to_range(microsecond, 0, 999);

    // 7. Set nanosecond to ! ConstrainToRange(nanosecond, 0, 999).
    nanosecond = constrain_to_range(nanosecond, 0, 999);

    // 8. Return the Record { [[Hour]]: hour, [[Minute]]: minute, [[Second]]: second, [[Millisecond]]: millisecond, [[Microsecond]]: microsecond, [[Nanosecond]]: nanosecond }.
    return TemporalTime { .hour = hour, .minute = minute, .second = second, .millisecond = millisecond, .microsecond = microsecond, .nanosecond = nanosecond };
}

// 4.5.8 CreateTemporalTime ( hour, minute, second, millisecond, microsecond, nanosecond [ , newTarget ] ), https://tc39.es/proposal-temporal/#sec-temporal-createtemporaltime
PlainTime* create_temporal_time(GlobalObject& global_object, u8 hour, u8 minute, u8 second, u16 millisecond, u16 microsecond, u16 nanosecond, FunctionObject* new_target)
{
    auto& vm = global_object.vm();

    // 1. Assert: hour, minute, second, millisecond, microsecond and nanosecond are integers.

    // 2. If ! IsValidTime(hour, minute, second, millisecond, microsecond, nanosecond) is false, throw a RangeError exception.
    if (!is_valid_time(hour, minute, second, millisecond, microsecond, nanosecond)) {
        vm.throw_exception<RangeError>(global_object, ErrorType::TemporalInvalidPlainTime);
        return {};
    }

    // 3. If newTarget is not present, set it to %Temporal.PlainTime%.
    if (!new_target)
        new_target = global_object.temporal_plain_time_constructor();

    // 4. Let object be ? OrdinaryCreateFromConstructor(newTarget, "%Temporal.PlainTime.prototype%", « [[InitializedTemporalTime]], [[ISOHour]], [[ISOMinute]], [[ISOSecond]], [[ISOMillisecond]], [[ISOMicrosecond]], [[ISONanosecond]], [[Calendar]] »).
    // 5. Set object.[[ISOHour]] to hour.
    // 6. Set object.[[ISOMinute]] to minute.
    // 7. Set object.[[ISOSecond]] to second.
    // 8. Set object.[[ISOMillisecond]] to millisecond.
    // 9. Set object.[[ISOMicrosecond]] to microsecond.
    // 10. Set object.[[ISONanosecond]] to nanosecond.
    // 11. Set object.[[Calendar]] to ! GetISO8601Calendar().
    auto* object = ordinary_create_from_constructor<PlainTime>(global_object, *new_target, &GlobalObject::temporal_plain_time_prototype, hour, minute, second, millisecond, microsecond, nanosecond, *get_iso8601_calendar(global_object));
    if (vm.exception())
        return {};

    // 12. Return object.
    return object;
}

// 4.5.9 ToTemporalTimeRecord ( temporalTimeLike ), https://tc39.es/proposal-temporal/#sec-temporal-totemporaltimerecord
Optional<TemporalTime> to_temporal_time_record(GlobalObject& global_object, Object& temporal_time_like)
{
    auto& vm = global_object.vm();

    // 1. Assert: Type(temporalTimeLike) is Object.

    // 2. Let result be the Record { [[Hour]]: undefined, [[Minute]]: undefined, [[Second]]: undefined, [[Millisecond]]: undefined, [[Microsecond]]: undefined, [[Nanosecond]]: undefined }.
    auto result = TemporalTime {};

    // 3. For each row of Table 3, except the header row, in table order, do
    for (auto& [internal_slot, property] : temporal_time_like_properties<TemporalTime, double>(vm)) {
        // a. Let property be the Property value of the current row.

        // b. Let value be ? Get(temporalTimeLike, property).
        auto value = temporal_time_like.get(property);
        if (vm.exception())
            return {};

        // c. If value is undefined, then
        if (value.is_undefined()) {
            // i. Throw a TypeError exception.
            vm.throw_exception<TypeError>(global_object, ErrorType::TemporalMissingRequiredProperty, property);
            return {};
        }

        // d. Set value to ? ToIntegerOrInfinity(value).
        auto value_number = value.to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};

        // e. Set result's internal slot whose name is the Internal Slot value of the current row to value.
        result.*internal_slot = value_number;
    }

    // 4. Return result.
    return result;
}

}
