DROP TABLE IF EXISTS answers;
DROP TABLE IF EXISTS scorer_question;
DROP TABLE IF EXISTS questions;
DROP TABLE IF EXISTS user_page;
DROP TABLE IF EXISTS pages;
DROP TABLE IF EXISTS users;

CREATE OR REPLACE FUNCTION update_timestamp()
    RETURNS TRIGGER AS
$$
BEGIN
    IF OLD.* IS DISTINCT FROM NEW.* THEN
        NEW.updated_at = CURRENT_TIMESTAMP;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE users
(
    id         SERIAL PRIMARY KEY,                                                                                 -- 自增主键
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP,                                              -- 创建时间，自动设置为当前时间
    updated_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP,                                              -- 更新时间，自动设置为当前时间
    username   VARCHAR(50)  NOT NULL UNIQUE,                                                                       -- 用户名，唯一且非空
    password   VARCHAR(255) NOT NULL       DEFAULT "$2a$04$Rs1mQfo5kzxHz.YwYolNWe26cm0BOfxciNdr2IIFjoM5zcvTnI2w6", -- 密码（推荐存储哈希值） 默认密码是123456
    permission INT          NOT NULL       DEFAULT 3,
    avatar     VARCHAR(500)                                                                                        -- 头像URL，可选字段
);
CREATE TRIGGER update_timestamp_trigger
    BEFORE UPDATE
    ON users
    FOR EACH ROW
EXECUTE PROCEDURE update_timestamp();

ALTER TABLE IF EXISTS public.users
    OWNER to postgres;

CREATE TABLE pages
(
    id         SERIAL PRIMARY KEY,                                    -- 自增主键
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 创建时间，自动设置为当前时间
    updated_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 更新时间，自动设置为当前时间
    title      TEXT NOT NULL,
    "desc"     TEXT,
    state      INT  NOT NULL               DEFAULT 3,
    start_at   TIMESTAMP WITHOUT TIME ZONE,
    end_at     TIMESTAMP WITHOUT TIME ZONE
);
ALTER TABLE IF EXISTS public.pages
    OWNER to postgres;
CREATE TRIGGER update_timestamp_trigger
    BEFORE UPDATE
    ON pages
    FOR EACH ROW
EXECUTE PROCEDURE update_timestamp();

-- CREATE TYPE question_type AS ENUM ('choose', 'fill_in', 'upload','none');

CREATE TABLE questions
(

    id         SERIAL PRIMARY KEY,                                    -- 自增主键
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 创建时间，自动设置为当前时间
    updated_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 更新时间，自动设置为当前时间
    title      TEXT             NOT NULL,
    "desc"     TEXT,
    "type"     question_type    NOT NULL,
    settings   jsonb,
    index      INT              NOT NULL,
    max_score  INT              NOT NULL   DEFAULT 10,
    score_step DOUBLE PRECISION NOT NULL   DEFAULT 1,

    page_id    INTEGER          NOT NULL REFERENCES pages (id)
);
ALTER TABLE IF EXISTS public.questions
    OWNER to postgres;
CREATE TRIGGER update_timestamp_trigger
    BEFORE UPDATE
    ON questions
    FOR EACH ROW
EXECUTE PROCEDURE update_timestamp();

CREATE TABLE answers
(
    id          SERIAL PRIMARY KEY,                                    -- 自增主键
    created_at  TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 创建时间，自动设置为当前时间
    content     TEXT,
    details     jsonb,

    score       INT,

    question_id INTEGER REFERENCES questions (id),
    answerer_id INTEGER REFERENCES users (id),
    scorer_id   INTEGER REFERENCES users (id)
);

ALTER TABLE IF EXISTS public.answers
    OWNER to postgres;

CREATE TABLE user_page
(
    id         SERIAL PRIMARY KEY,                                    -- 自增主键
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 创建时间，自动设置为当前时间
    user_id    INTEGER REFERENCES users (id),
    page_id    INTEGER REFERENCES pages (id),
    UNIQUE (user_id, page_id)
);

ALTER TABLE IF EXISTS public.user_page
    OWNER to postgres;


CREATE TABLE scorer_question
(
    id          SERIAL PRIMARY KEY,                                    -- 自增主键
    created_at  TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP, -- 创建时间，自动设置为当前时间
    user_id     INTEGER REFERENCES users (id),
    question_id INTEGER REFERENCES questions (id),
    UNIQUE (user_id, question_id)
);
ALTER TABLE IF EXISTS public.scorer_question
    OWNER to postgres;