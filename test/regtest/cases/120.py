description = "test breadcrumb"

filedata = """
{$
    print (breadcrumb () + " ");
    locals { o : eval_location () }
    print (o.lineno);
$}"""

outcome = "/regtest-scratch/cases_120.html:3 4"
