Param(
    [int]$Rows = 1000000,
    [string]$Db = "E:\Code\Project_grade3\SX\new\miniBase\data\mini.db",
    [string]$Cli = "E:\Code\Project_grade3\SX\new\miniBase\build\src\cli\Debug\minidb_cli.exe",
    [string]$OutDir = "E:\Code\Project_grade3\SX\new\miniBase\bench_out",
    [int]$Runs = 5,
    [int]$UserId = 12345
)

$ErrorActionPreference = "Stop"
[System.IO.Directory]::CreateDirectory($OutDir) | Out-Null
$dataset = Join-Path $OutDir ("bigdata_" + $Rows + ".sql")
$logImport = Join-Path $OutDir "import.log"
$csv = Join-Path $OutDir ("bench_" + $Rows + ".csv")

Write-Host "[1/6] Generating data: $Rows rows -> $dataset"
# "DROP TABLE orders;" | Set-Content $dataset -Encoding UTF8
"CREATE TABLE orders (id INT, user_id INT, amount DOUBLE, status VARCHAR(10));" | Add-Content $dataset -Encoding UTF8
0..($Rows-1) | ForEach-Object {
  $id = $_
  $uid = Get-Random -Minimum 1 -Maximum 100000
  $amt = [math]::Round((Get-Random -Minimum 1 -Maximum 100000)/100.0,2)
  $st = @("NEW","PAID","CANCEL","REFUND")[ (Get-Random -Minimum 0 -Maximum 4) ]
  "INSERT INTO orders VALUES('$id', '$uid', '$amt', '$st');"
} | Add-Content $dataset -Encoding UTF8

# Helper to run a query once
function Invoke-MiniDBQuery([string]$sql){
  @"
root
root
$sql
.exit
"@ | & $Cli --exec --db $Db | Out-Null
}

Write-Host "[2/6] Importing into $Db"
# 先用 CLI 管道清理旧表（Importer 不支持 DROP）
Invoke-MiniDBQuery "DROP TABLE orders;"
@"
root
root
.import $dataset
.exit
"@ | & $Cli --exec --db $Db | Tee-Object -FilePath $logImport | Out-Null

# Warmup
Write-Host "[3/6] Warmup run"
Invoke-MiniDBQuery "SELECT * FROM orders WHERE user_id='$UserId';"

# Benchmark without index
Write-Host "[4/6] Benchmarking no-index"
$noIndex = @()
for($i=1;$i -le $Runs;$i++){
  $t = Measure-Command { Invoke-MiniDBQuery "SELECT * FROM orders WHERE user_id='$UserId';" }
  $noIndex += [pscustomobject]@{ phase='no_index'; run=$i; ms=$t.TotalMilliseconds }
  Write-Host ("  no_index run {0}: {1} ms" -f $i, [int]$t.TotalMilliseconds)
}

# Create index（不使用 IF NOT EXISTS，避免解析错误）
Write-Host "[5/6] Creating index"
Invoke-MiniDBQuery "CREATE INDEX idx_orders_user_id ON orders(user_id);"

# Benchmark with index
Write-Host "[6/6] Benchmarking with-index"
$withIndex = @()
for($i=1;$i -le $Runs;$i++){
  $t = Measure-Command { Invoke-MiniDBQuery "SELECT * FROM orders WHERE user_id='$UserId';" }
  $withIndex += [pscustomobject]@{ phase='with_index'; run=$i; ms=$t.TotalMilliseconds }
  Write-Host ("  with_index run {0}: {1} ms" -f $i, [int]$t.TotalMilliseconds)
}

$all = $noIndex + $withIndex
$all | Export-Csv -NoTypeInformation -Encoding UTF8 $csv
Write-Host "Done. Results -> $csv"
