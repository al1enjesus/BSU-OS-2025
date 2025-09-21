journalctl -p err | 
     tr -cs '[:alnum:]' '\n' |
     tr '[:upper:]' '[:lower:]' |
     sort |
     uniq -c |
     sort -nr | 
     head -n 5
