/*
 * @Author: peng.cao
 * @Date: 2021-05-07 21:00:12
 * @LastEditTime: 2021-05-07 21:02:10
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /tinyhttp/tinyhttp.cpp
 */

#include <stdio.h>
#include <httpd.h>

int main(int argc, char ** argv)
{
    Httpd http;
    LOG("startup port: %d", 8080);
    http.startup(8080);
    return 0;
}