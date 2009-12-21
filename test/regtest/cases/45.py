#
# simple test of if statements with different spacings
#


filedata="""
{$

    if (false)  {{ no }}
    else        {{ yes }}

   locals 
       
         { 

      inner 
 
       : 

       "hello" 

     }

   locals {
        js: "print this 
              $('${inner}').js_function" }
$}

${js}

"""

outcome="""
yes
print this $('hello').js_function
"""

desc = "test spacing possibilities"

