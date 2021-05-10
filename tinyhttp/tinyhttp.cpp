/*
 * @Author: peng.cao
 * @Date: 2021-05-07 21:00:12
 * @LastEditTime: 2021-05-08 23:11:31
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /tinyhttp/tinyhttp.cpp
 */

#include <stdio.h>
#include <http.h>

int main(int argc, char ** argv)
{
    Http http;
    LOG("startup port: %d", 8080);
    http.startup(8080);
    return 0;
}