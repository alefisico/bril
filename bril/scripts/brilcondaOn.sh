prepend_path_part(){
 if echo ":$PATH:" | grep -q ":$1:"; then echo "$PATH"; else echo "$1:$PATH"; fi
}
append_path_part(){
 if echo ":$PATH:" | grep -q ":$1:"; then echo "$PATH"; else echo "$PATH:$1"; fi
}
PATH=$(prepend_path_part /opt/brilconda/bin)
echo "PATH is " ${PATH}