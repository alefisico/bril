remove_path_part(){
  echo ":$PATH:" | sed "s@:$1:@:@g;s@^:\(.*\):\$@\1@"
}
PATH=$(remove_path_part /opt/brilconda/bin)
echo "PATH is " ${PATH}