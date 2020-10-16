readFileName = "sample-%d.bin"
writeFileName = "copy-sample-%d.bin"
resultFileName = "result-sample-%d-12.py"

threads = 12
with open("samples.bat", "w") as smpls:
    
    i = 7
#    for i in range(5):
#    for j in range(17):
    curRes = resultFileName % i
    smpls.write('echo data = {> %s\n' % curRes)
    j = 18
    smpls.write('echo (%d, %d, %d): [>> %s\n' %(threads, 2**j, 512 * 8 ** i, curRes))
    for _ in range(10):
        smpls.write('..\\bin\\cocopypy %s %s %d %d >> %s\n' %((readFileName % i), (writeFileName % i), threads, 2**j, curRes))
        smpls.write('echo , >> %s\n' % curRes)
    smpls.write('echo ],>> %s\n' % curRes)
    smpls.write('echo }>> %s\n' % curRes)
