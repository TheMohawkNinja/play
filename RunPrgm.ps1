param([string] $p="", [string] $user="", [string] $task="", [string] $name="")
$dir=$p.Substring(0,$p.LastIndexOf('/'))
$prgm=$p.Substring(($dir.Length+1),($p.Length-$dir.Length-1))
$ID=(query session | grep $user | grep -o [0-9])

#Start game
if($task -eq "start")
{
    #Check if path is a steam gameid path or a true program path
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
    echo dir=$dir
    #Check if path is a steam gameid path or a true program path
    if($dir -eq "steam://rungameid")
    {
        echo Steam

        #Basically everything from here to setting the $wpid variable is to allow the curl command to handle the fact that Steam is https
        #C# class to create callback
        $code = @"
        public class SSLHandler
        {
            public static System.Net.Security.RemoteCertificateValidationCallback GetSSLHandler()
            {

                return new System.Net.Security.RemoteCertificateValidationCallback((sender, certificate, chain, policyErrors) => { return true; });
            }

        }
"@

        #compile the class
        Add-Type -TypeDefinition $code

        #disable checks using new class
        [System.Net.ServicePointManager]::ServerCertificateValidationCallback = [SSLHandler]::GetSSLHandler()
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

        #do the request
        try
        {
            echo "Trying to parse name from Steam Store..."
            echo "gameid=$prgm"
            $name=curl -uri https://store.steampowered.com/app/$prgm | Select-Object -Expand Content | grep -o -P '(?<=<title>).*(?= on Steam</title>)'
        }
        catch
        {
            # do something
        }
        finally
        {
           #enable checks again
           [System.Net.ServicePointManager]::ServerCertificateValidationCallback = $null
        }
        $wpid=cmd /s /c "tasklist -v | grep -i '$name' | grep -o -E '[0-9]{4,}'"
        echo name=$name
    }
    else
    {
        $wpid=cmd /s /c "tasklist -v | grep -i '$prgm' | grep -o -E '[0-9]{4,}'"
        echo prgm=$prgm
    }
    echo pid=$wpid
    kill -Force $wpid
}
























