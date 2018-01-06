#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Jan  6 20:19:54 2018

@author: anatolyilin
"""
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

import matplotlib.dates as mdates
import datetime
# read file
dataset = pd.read_csv('~/Desktop/measurements.txt',   names = ["timestamp", "T", "H", "Date", "Time"])


# get rid of nan's, by overwriting them with previous value:
dataset.fillna(method='ffill', inplace=True)

# get rid of values < 100 from the first colom, e.g. when time sync failed
dataset = dataset[dataset['timestamp'] > 1000 ]

# convert unix timestamp to datatime format
dataset['timestamp'] = pd.to_datetime(dataset['timestamp'],unit='s')

# formatters. 

def plotDaily6h():
    years = mdates.YearLocator()   
    months = mdates.MonthLocator() 
    days = mdates.DayLocator()
    hours = mdates.HourLocator(byhour=[0,6,12,18], interval = 1)
    minutes = mdates.MinuteLocator()
    seconds = mdates.SecondLocator()
    yearsFmt = mdates.DateFormatter('%d-%m')

    yearsFmt = mdates.DateFormatter('%d-%m')
    fig, (T,H) = plt.subplots(2, sharex=True)
    T.plot(dataset['timestamp'], dataset['T'], color="blue", label="Temperature ['C]")
    T.grid(True)
    
    H.plot(dataset['timestamp'], dataset['H'], color="red", label="Rel. Humidity [%]")
    H.xaxis.set_major_locator(days)
    H.xaxis.set_major_formatter(yearsFmt)
    H.xaxis.set_minor_locator(hours)
    H.grid(True)
    fig.legend()
    plt.show()

def plotHourly30m():
    hours = mdates.HourLocator()
    minutes = mdates.MinuteLocator(byminute=[0,30], interval = 1)
    seconds = mdates.SecondLocator()
    yearsFmt = mdates.DateFormatter('%H')
    fig, (T,H) = plt.subplots(2, sharex=True)
    T.plot(dataset['timestamp'], dataset['T'], color="blue", label="Temperature ['C]")
    T.grid(True)
    
    H.plot(dataset['timestamp'], dataset['H'], color="red", label="Rel. Humidity [%]")
    H.xaxis.set_major_locator(hours)
    H.xaxis.set_major_formatter(yearsFmt)
    H.xaxis.set_minor_locator(hours)
    H.grid(True)
    fig.legend()
    plt.show()
    
plotHourly30m()