#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import re

def main():
    # 打开文件
    fi = open("input.txt", "r")
    print("文件名为: ", fi.name)
    fo = open(sys.argv[1], "r", errors="surrogateescape")
    print("文件名为: ", fo.name)

    duration_times = {}
    line_count = 0

    for iline in fi.readlines(): #依次读取每行
        print(iline)
        sum_s = 0
        sum_us = 0
        count = 0
        sum_s2 = 0
        sum_us2 = 0
        count2 = 0
        if duration_times.__contains__(iline):
            sum_s, sum_us, count, sum_s2, sum_us2, count2 = duration_times[iline]
        else:
            duration_times[iline] = (sum_s, sum_us, count, sum_s2, sum_us2, count2)
        while True:
            oline=fo.readline()
            if not oline:
                break
            line_count += 1
            if oline.find("该功能总执行时间") >= 0:
                print(oline, end='')
                pattern = "\d+s \d+us"
                s = re.findall(pattern, oline)
                seconds, useconds = re.findall("\d+", s[0])
                sum_s2 += int(seconds)
                sum_us2 += int(useconds)
                count2 += 1
                print(s[0])
                break
            elif oline.find("执行时间") >= 0:
                print(oline, end='')
                pattern = "\d+s \d+us"
                s = re.findall(pattern, oline)
                seconds, useconds = re.findall("\d+", s[0])
                sum_s += int(seconds)
                sum_us += int(useconds)
                count += 1
                print(s[0])
                #print(seconds, useconds)
        duration_times[iline] = (sum_s, sum_us, count, sum_s2, sum_us2, count2)
        print(sum_s, sum_us, count, sum_s2, sum_us2, count2, '\n')

    # 关闭文件
    fo.close()

    print("行数" , line_count)

    for k in duration_times.keys():
        sum_s, sum_us, count, sum_s2, sum_us2, count2 = duration_times[k]
        if 0 == count:
            print(k, '函数运行次数:', count, ' ', sum_s, 's ', sum_us, 'us', sep='')
            continue
        print(k, '函数运行次数:', count, ' ', sum_s/count, 's ', sum_us/count, 'us', sep='')
        if 0 == count2:
            print('功能运行次数:', count2, ' ', sum_s2, 's ', sum_us2, 'us', sep='')
            continue
        print('功能运行次数:', count2, ' ', sum_s2/count2, 's ', sum_us2/count2, 'us', sep='')



if __name__ == "__main__":
    main()