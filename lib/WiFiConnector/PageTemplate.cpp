#include <Arduino.h>

const char _HTMLPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang=\"en\">
    <head>
        <title>{{pageTitle}}</title>
        <meta charset=\"utf-8\">
        <meta name=\"viewport\" content=\"width=device-width\">
        <style>
            * { box-sizing: border-box; }
            body { margin: 0px; font-family: sans-serif; font-size: 2rem; padding-bottom: 2rem; }
            a { text-decoration: none; color: #fff; }
            header { background-color: #222; color: #fff; text-align: center; text-transform: uppercase; padding: 2rem; margin-bottom: 2rem; }
            h1 { font-weight: normal; }
            article { margin: auto; max-width: 80%; }
            .button { cursor: hand; padding: 2rem; color: #fff; display: block; width: 100%; background-color: #5a3859; margin: 1rem 0 1rem 0; text-align: center; font-size: 2rem; }
            label { display: block; padding: 2px; margin-top: 1rem; }
            .textbox { display:block; padding: .5rem; width: 100%; font-size: 2rem; }
        </style>
        </head>
        <body>
            <header>
                <h1><a href="/">{{pageTitle}}</a></h1>
            </header>
            <article>
                {{pageContent}}
            </article>
            <script language="javascript">
                function validateForm() {
                    if (document.getElementByID("ssid").value == "" || document.getElementByID("devId").value == "") {
                        alert("SSID and Device name fields cannot be blank");
                        return false;
                    }
                }
            </script>
        </body>
        </html>
)=====";