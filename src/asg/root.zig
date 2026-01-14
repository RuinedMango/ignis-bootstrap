const std = @import("std");

const NType = enum {
    MODULE,
    SCOPE,
    FUNCTION,
    VARIABLE,
    TYPE,
    STRUCT,
    ENUM,
    ENUM_CASE,
    FIELD,
    GENERIC_PARAM,
    COUNT,
};

const Node = struct {
    type: NType,
    payload: u32,
};

const EType = enum {
    DECLARES,
    REFERENCES,
    HAS_TYPE,
    CONTAINS,
    INHERITS,
    IMPLEMENTS,
    COUNT,
};

const Edge = struct {
    from: u32,
    to: u32,
};

const ASG = struct {};
