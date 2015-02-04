// -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
typedef u_int64_t userid_t;
typedef int       ok_time_t;

struct blob_vote_tally_t {
    unsigned weighted_sum;
    unsigned num_votes;
};

struct uid_wrapper_t {
    userid_t userid;
};

enum blob_vers_t {
    BV_NONE = 0,
    BV_V1 = 1,
    BV_V2 = 2
};

struct blob_1_t {
    userid_t userid;

    ok_time_t last_login;

    unsigned birth_year;
    unsigned birth_day;

    unsigned gender;
    unsigned orientation;

    unsigned religion;
    unsigned religionserious;

    unsigned usemetric;
    unsigned height;
    unsigned smoking;
    unsigned drinking;
    unsigned drugs;
    unsigned money;
    unsigned jobtype;
    unsigned ethnicity;
    unsigned bodytype;

    unsigned *opt_val;
};

struct blob_2_t {

    // profile data
    userid_t userid;

    ok_time_t last_login;

    unsigned birth_year;
    unsigned birth_day;

    unsigned gender;
    unsigned orientation;
    unsigned gentation;
    unsigned gentation_want;

    unsigned picture_active;

    unsigned religion;
    unsigned religionserious;

    unsigned usemetric;
    unsigned height;
    unsigned smoking;
    unsigned drinking;
    unsigned drugs;
    unsigned money;
    unsigned jobtype;
    unsigned ethnicity;
    unsigned bodytype;

    unsigned languages<>;

    unsigned looking_for;
    unsigned relationship_status;

    unsigned dogs;
    unsigned cats;

    unsigned children;

    unsigned education_status;
    unsigned education_level;

    unsigned sign;
    unsigned sign_status;

    unsigned acct_status;
    unsigned acct_level;
    unsigned prev_acct_levels;

    unsigned profile_score;
    unsigned other_settings;

    // Personality stuff
    unsigned int personality_code;
    blob_vote_tally_t personality_votes;
    blob_vote_tally_t messaging_votes;
    blob_vote_tally_t date_votes;

    userid_t hidden_users<>;
    userid_t users_hiding_me<>;

    int32_t grid_latitude;
    int32_t grid_longitude;
    u_int32_t locid;

    opaque minis<>;

    ok_time_t join_date;

    unsigned diet;
    unsigned diet_serious;

};

union blob_t switch (blob_vers_t ver) {
 case BV_V1:
     blob_1_t v1;
 case BV_V2:
     blob_2_t v2;
 default:
     void;
};
