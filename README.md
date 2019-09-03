WXInlinePlayer (Version 1.2)
------------------
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)

* [简介]()
* [特性]()
* [示例]()
* [快速开始]()
* [API]()
* [事件]()
* [初始化参数]()
* [如何选择解码依赖]()
* [如何降低卡顿和延迟]()
* [其他问题]()
* [项目计划]()
* [已知使用的产品]()
* [QQ技术支持群]()

## 简介

随着直播和短视频的兴起，视频由于承担了更大的信息量，因此现在已经是非常主流的运营/产品信息输出方式。但由于国内各个浏览器厂商自身的利益关系所在，他们对HTML5的Video能力做了非常多的限制，不限于：
1. 禁止自动播放
2. 播放器魔改为原生播放器，层级最高，不可进行HTML相关元素层叠
3. 播放前后硬插广告内容
4. 视频自动置顶
5. 相关API和事件实现不统一
6. ......

其具体问题可以参考腾讯IMWeb团队编写的[《复杂帧动画之移动端Video采坑实现》](https://juejin.im/post/5d513623e51d453b72147600)。

为了解决这些问题，我们通过软解FLV的方式实现了WXInlinePlayer，其用的第三方技术和平台API如下：
1. [OpenH264](https://github.com/cisco/openh264)/[TinyH264](https://github.com/udevbe/tinyh264)；
2. [Emscripten](https://github.com/emscripten-core/emscripten)
3. [WebGL](https://developer.mozilla.org/zh-CN/docs/Web/API/WebGL_API)
4. [Web Audio Api](https://developer.mozilla.org/zh-CN/docs/Web/API/Web_Audio_API)

同时我们也编写了WebAssembly版本的FLV Demuxer，你可以在[lib/codec](https://github.com/qiaozi-tech/WXInlinePlayer/tree/master/lib/codec)找到相关代码。

## 特性

1. FLV点播/直播全支持
2. 自由选择H264解码依赖，Tinyh264只需gzip ~180k，OpenH264依赖gzip ~260k （[如何选择解码依赖]()）
3. 专为移动端优化性能，内存和CPU占用稳定
4. 直播延迟优化，比MSE的原生Video实现低1-2s（[如何降低卡顿和延迟]()）
5. 音频/视频全支持
6. 微信WebView自动播放
7. 无音频动画自动播放
8. 良好的移动端WebView兼容性

## 示例
> https://qiaozi-tech.github.io/WXInlinePlayer/example/index.html

## 快速开始
```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
  <title>WXInlinePlayer</title>
  <style>
    * {
      margin: 0;
      padding: 0;
    }

    html,
    body {
      width: 100%;
      height: 100%;
    }
  </style>
</head>
<body>
  <canvas id="container" width="800" height="450"></canvas>
  <script src="./index.js"></script>
  <script>
    if (WXInlinePlayer.isSupport()) {
      WXInlinePlayer.init({
        asmUrl: './prod.baseline.asm.combine.js',
        wasmUrl: './prod.baseline.wasm.combine.js'
      });

      WXInlinePlayer.ready().then(() => {
        const player = new WXInlinePlayer({
          url: 'https://static.petera.cn/mm.flv',
          $container: document.getElementById('container'),
          hasVideo: true,
          hasAudio: true,
          volume: 1.0,
          muted: false,
          autoplay: true,
          loop: true,
          isLive: false,
          chunkSize: 128 * 1024,
          preloadTime: 5e2,
          bufferingTime: 1e3,
          cacheSegmentCount: 64,
          customLoader: null
        });

        const { userAgent } = navigator;
        const isWeChat = /MicroMessenger/i.test(userAgent);
        if (!isWeChat) {
          alert('click to play!');
          document.body.addEventListener('click', () => {
            player.play();
          });
        }
      });
    }
  </script>
</body>
</html>
```

## API

### Boolean WXInlinePlayer.isSupport(void)

当前执行环境是否支持WXInlinePlayer。
```javascript
if(WXInlinePlayer.isSupport()){
  console.log('WXInlinePlayer support');
}
```

### Promise WXInlinePlayer.init(Object)

初始化WXInlinePlayer，需要传入加载的H264解码库的具体地址，关于解码库的选择，请参考：[如何选择解码依赖]()。
```javascript
if(WXInlinePlayer.isSupport()){
  WXInlinePlayer.init({
    asmUrl: './prod.baseline.asm.combine.js',
    wasmUrl: './prod.baseline.wasm.combine.js'
  }).catch(e=>{
    console.log(`WXInlinePlayer init error: ${e}`);
  });
}
```

### Promise WXInlinePlayer.ready(void)

WXInlinePlayer已经准备就绪，可以安全的进行初始化操作。

```javascript
if(WXInlinePlayer.isSupport()){
  WXInlinePlayer.init({/*.....*/});
  WXInlinePlayer.ready().then(()=>{
    console.log('WXInlinePlayer ready');
  });
}
```

### WXInlinePlayerInstance WXInlinePlayer(Object)

WXInlinePlayer构造函数，相关初始化参数请参考：[初始化参数]()。

```javascript
WXInlinePlayer.ready().then(()=>{
  const player = new WXInlinePlayer({/*...*/});
});
```

### void WXInlinePlayerInstance.play(void)

进行视频播放。需要注意的是由于浏览器限制（不包含微信），高版本已经禁用了音频自动播放，因此直接调用此方法可能并不会有作用，请在click/touchstart/touchend/touchmove等事件中让用户主动触发。

```javascript
const player = new WXInlinePlayer({/*...*/});
document.body.addEventListener('click', ()=>{
  player.play();
});
```

### void WXInlinePlayerInstance.stop(void)

停止整个播放器，不可被恢复(resume)。

```javascript
const player = new WXInlinePlayer({/*...*/});
player.stop();
```

### void WXInlinePlayerInstance.pause(void)

暂停当前播放。

```javascript
const player = new WXInlinePlayer({/*...*/});
player.pause();
```

### void WXInlinePlayerInstance.resume(void)

恢复由pause引起的暂停操作。

```javascript
const player = new WXInlinePlayer({/*...*/});
player.resume();
```

### Number|void WXInlinePlayerInstance.volume(Number|void)

获取/设置当前音量。
```javascript
const player = new WXInlinePlayer({/*...*/});
const volume = player.volume(); // get volume
player.volume(volume); // set volume
```

### Boolean|void WXInlinePlayerInstance.mute(Boolean|void)

获取/设置静音状态。
```javascript
const player = new WXInlinePlayer({/*...*/});
const muted = player.mute(); // get mute
player.mute(muted); // set mute
```

### void WXInlinePlayerInstance.destroy(void)

销毁播放器，释放所有内存等待回收。
```javascript
const player = new WXInlinePlayer({/*...*/});
player.destroy();
```

### Number WXInlinePlayerInstance.getCurrentTime(void)

获取当前播放时间，请注意，可能出现负值的情况请注意处理。
```javascript
const player = new WXInlinePlayer({/*...*/});
player.on('timeUpdate', ()=>{
  let currentTime = player.getCurrentTime();
  currentTime = currentTime <= 0 ? 0 : currentTime;
});
```

### Number WXInlinePlayerInstance.getAvaiableDuration(void)

可播放时长，可理解为缓冲的时长。
```javascript
const player = new WXInlinePlayer({/*...*/});
player.on('timeUpdate', ()=>{
  const duration = player.getAvaiableDuration();
});
```

## 事件

* mediaInfo(Object) - 视频相关信息，例如width/height/fps/framerate等
* playing(void) - 开始/正在播放
* buffering(void) - 内部帧数据不足，开始缓冲
* stopped(void) - 停止播放
* end(void) - 播放结束
* timeUpdate(currentTime:Number) - 当前播放的进度，250ms进行一次触发

## 初始化参数
```javascript
{
  url: String, // 播放地址，仅支持flv
  $container: DomElement, // 绘制的canvas对象
  hasVideo: Boolean, // 是否含有视频，默认true
  hasAudio: Boolean, // 是否含有音频，默认true
  volume: Number, // 音量，范围0~1，默认1.0
  muted: Boolean, // 是否静音，默认false
  autoplay: Boolean, // 是否自动播放，微信WebView/无音频视频设置此参数有效
  loop: Boolean, // 循环播放，默认false
  isLive: Boolean, // 是否是直播流，默认false
  chunkSize: Number, // 加载/解析块大小，默认256 * 1024
  preloadTime: Number, // 预加载时间，默认1000ms
  bufferingTime: Number, // 缓冲时间，默认3000ms
  cacheSegmentCount: Number, // 内部最大的缓存帧数量，默认256
  customLoader: LoaderImpl, // 自定义loader，请参考src/loader/chunk(stream)代码
}
```

## 如何选择解码依赖

目前有两套解码库，分别是：
* prod.baseline.asm.combine / prod.baseline.wasm.combine
* prod.all.asm.combine / prod.all.wasm.combine

其区别在于：
1. baseline文件大小更小（gzip后相比all小80k），但是只支持baseline的profile
2. all的profile支持更完整（baseline/main/high），并且性能相比于baseline更好

我们推荐当你播放广告视频/营销视频/小动画视频等对依赖库大小敏感的时候使用baseline.asm/baseline.wasm，而在播放点播视频/直播视频时等对依赖库大小不敏感的时候使用all.asm/all.wasm。

## 如何降低卡顿和延迟

WXInlinePlayer的卡顿和延迟主要来自于3个地方：
* 网络加载的延迟
* 软解码的延迟
* 渲染的延迟

一般来说，如果在用户网络环境较好的情况，渲染由于使用了WebGL，因此很难造成瓶颈（操作很单一），其中比较常见的问题会出现在软解码性能不足造成不停卡顿及延迟。

优化由于软解码造成的延迟我们从如下几个地方着手：
1. 视频的profile：相比于main/high而言，baseline不包含B帧，解码消耗更低
2. 视频帧率：过高的帧率会造成软解码跟不上，可以试着降低帧率，例如24fps
3. 视频码率：码率越高，视频富含的细节越多，也越清晰，但是会消耗更多的解码性能，可以试着降低码率
4. 视频分辨率：过高的视频会造成单帧传递的数量极大，目前WXInlinePlayer在中端机上解1280x720，码率1024，帧率24fps的视频比较流畅

以上参数你可以通过ffmpeg查看：
```shell
ffmpeg -i "your.flv"
```

在这里我们给出主流平台的profile/帧率/码率/分辨率供参考：

平台 | 类型 | 清晰度 | profile | 帧率 | 码率 | 分辨率
 -|-|-|-|-|-|-
虎牙|横屏|标清|High|24|500k|800x450
虎牙|横屏|高清|High|24|1200k|1280x720
虎牙|竖屏|高清|Main|16|1280k|540x960
奇秀|竖屏|标清|High|15|307k|204x360
奇秀|竖屏|高清|High|15|512k|304x540
奇秀|竖屏|超清|Baseline|15|1440k|720x1280
抖音|竖屏|默认|High|30|1600k（变化较多，仅供参考）|720x1280
快手|竖屏|默认|High|25|2880k（变化较多，仅供参考）|720x1280

我们建议你：
1. 如果你想能够覆盖更多的机型，那么奇秀标清或是高清的配置适合你
2. 如果你想只支持Android中端机和iPhone6+，那么虎牙高清的配置适合你

## 其他问题
1. 为什么不对FFmpeg精简后Emscripten编译？
2. 为什么有些机器播放点播/直播会频繁卡顿，如何解决？
3. 为什么不对UC浏览器（iOS/Android）进行支持？
4. 如何将现有视频文件转换成WXInlinePlayer可播放的文件？

## 项目计划
1. <del>V1.1 支持HTTP-FLV及流式解码</del>
2. <del>V1.1 支持音视频独立播放</del>
3. <del>V1.2 降低直播流延迟</del>
4. V1.3 重构解码器，精确缓存帧数据
5. V1.4 提供默认的播放器UI
6. V1.5 支持FLV seek操作
7. V2.0 支持多Worker解码，提升软解性能

## 已知使用的产品

* [好惠买](http://h5.haohuimai1.com/)
* [兔几直播](https://www.tuji.com/)

## QQ技术支持群
![QQ群](./images/qq.jpeg "QQ群")