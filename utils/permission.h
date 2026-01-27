#pragma once
#include <concepts>
#include <typeinfo>
#include <asio/prefer.hpp>
#include <asio/require.hpp>

namespace permission {
    inline bool has_permission(int64_t target, int64_t required) {
        return (target & required) == required;
    }

    enum user_permission {
        login = 1,
        admin = 2,
        view_task = 4,
        submit = 8,
        manage_task = 16,
        upload = 32,
        score = 64,
    };


    enum task_permission {
        enable = 1,
        // 用户是否可见
        visible = 2,
        // 可见但不可进入
        readable = 4,
        // 不可提交
        submittable = 8,
        // 不可更改记录
        changeable = 16,

        // 审阅者是否可见
        reviewer_visible = 32,
        // 是否可审阅
        reviewable = 64,
    };
}

