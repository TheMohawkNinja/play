param([string] $p="", [string] $user="", [string] $task="", [string] $name="")
$dir=$p.Substring(0,$p.LastIndexOf('/'))
$prgm=$p.Substring(($dir.Length+1),($p.Length-$dir.Length-1))
$ID=(query session | grep $user | grep -o [0-9])

#Start game
if($task -eq "start")
{
    #Check if path is a steam gameid path or a true program path
    echo "$dir \n"
    if($dir -eq "steam://rungameid")
    {
        psexec -nobanner -accepteula -i $ID -s cmd /s /c "start steam://rungameid/$prgm"
    }
    #Need to cd and then call the program due to occassional file permission weirdness
    else
    {
        psexec -nobanner -accepteula -i $ID -s cmd /s /c "cd $dir && $prgm"
    }
}
elseif($task -eq "kill")
{
    $wpid=cmd /s /c "tasklist -v | grep -i $name | grep -o -E '[0-9]{4,}'"
    kill -Force $wpid
}