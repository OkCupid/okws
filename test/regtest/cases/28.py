desc = "basic test of assignments (case 2) (30.py in OKWS 2.1)"

filedata = ["""
{$
    locals { none = 0,
          lotame = 1,
          rubi = 2 } ;

    locals { none = 0,
          skyscrape = 1,
          leaderboard = 2 };

    locals { files : {} }

    files[lotame] = {};
    files[lotame][skyscrape] = "$[1]";
    files[lotame][leaderboard] = "$[2]";
    files[rubi] = {};
    files[rubi][skyscrape] = "$[3]";
    files[rubi][leaderboard] = "$[4]";

    include (files[lotame][skyscrape])
    include (files[rubi][leaderboard])
$}
""",
"lotame skyscraper ",
"lotame leaderboard ",
"rubi skyscraper ",
"rubi leaderboard "
]

outcome = "lotame skyscraper rubi leaderboard"


