# 像素跑酷

## 项目简介
- 一个既包含跑酷元素有拥有战斗系统的游戏，画风精美，音效有趣
- A/D/W/S控制移动方向，W/K都能够实现跳跃，支持二段跳，J可以攻击

## 使用方法
- 双击进入压缩包或解压后进入文件双击含游戏名文件，即可进入游戏

## 外部库
- 使用在网上找到的透明贴图背景的函数库
''' c
#pragma once
#include<easyx.h>

void drawImg(int x, int y, IMAGE* src)
{
	// 变量初始化
	DWORD* pwin = GetImageBuffer();			//窗口缓冲区指针
	DWORD* psrc = GetImageBuffer(src);		//图片缓冲区指针
	int win_w = getwidth();				//窗口宽高
	int win_h = getheight();
	int src_w = src->getwidth();				//图片宽高
	int src_h = src->getheight();

	// 计算贴图的实际长宽
	int real_w = (x + src_w > win_w) ? win_w - x : src_w;			// 处理超出右边界
	int real_h = (y + src_h > win_h) ? win_h - y : src_h;			// 处理超出下边界
	if (x < 0) { psrc += -x;			real_w -= -x;	x = 0; }	// 处理超出左边界
	if (y < 0) { psrc += (src_w * -y);	real_h -= -y;	y = 0; }	// 处理超出上边界


	// 修正贴图起始位置
	pwin += (win_w * y + x);

	// 实现透明贴图
	for (int iy = 0; iy < real_h; iy++)
	{
		for (int ix = 0; ix < real_w; ix++)
		{
			byte a = (byte)(psrc[ix] >> 24);//计算透明通道的值[0,256) 0为完全透明 255为完全不透明
			if (a > 100)
			{
				pwin[ix] = psrc[ix];
			}
		}
		//换到下一行
		pwin += win_w;
		psrc += src_w;
	}
}

void drawImg(int x, int y, int dstW, int dstH, IMAGE* src, int srcX, int srcY)
{
	// 变量初始化
	DWORD* pwin = GetImageBuffer();			//窗口缓冲区指针
	DWORD* psrc = GetImageBuffer(src);		//图片缓冲区指针
	int win_w = getwidth();				//窗口宽高
	int win_h = getheight();
	int src_w = src->getwidth();				//图片宽高
	int src_h = src->getheight();


	// 计算贴图的实际长宽
	int real_w = (x + dstW > win_w) ? win_w - x : dstW;			// 处理超出右边界
	int real_h = (y + dstH > win_h) ? win_h - y : dstH;			// 处理超出下边界
	if (x < 0) { psrc += -x;			real_w -= -x;	x = 0; }	// 处理超出左边界
	if (y < 0) { psrc += (dstW * -y);	real_h -= -y;	y = 0; }	// 处理超出上边界

	//printf("realw,h(%d,%d)\n", real_w, real_h);
	// 修正贴图起始位置
	pwin += (win_w * y + x);

	// 实现透明贴图
	for (int iy = 0; iy < real_h; iy++)
	{
		for (int ix = 0; ix < real_w; ix++)
		{
			byte a = (byte)(psrc[ix + srcX + srcY * src_w] >> 24);//计算透明通道的值[0,256) 0为完全透明 255为完全不透明
			if (a > 100)
			{
				pwin[ix] = psrc[ix + srcX + srcY * src_w];
			}
		}
		//换到下一行
		pwin += win_w;
		psrc += src_w;
	}
}
'''
## 难点与解决方案
1. 只有角色单方向的动作精灵图，编辑图片只能改变整体图片的朝向
	
2. 角色并未与障碍物或敌人相接触，但是却判定游戏结束
  解决方案：绘制相关的碰撞盒，可视化碰撞检测，通过调整碰撞盒的大小，进一步调整碰撞判定范围
3. 使用srand((unsigned int)time(NULL))随机生成的平台并非真正的随机，在每次按R重新开始游戏时，都与上第一次运行程序的地图一样 
  解决方案：时间戳加程序运行时间，增强随机性srand((unsigned int)time(NULL) + clock()) 这样能够每次按下R键重新开始时看见不同的地图                                                                                                                                                                                                                                              