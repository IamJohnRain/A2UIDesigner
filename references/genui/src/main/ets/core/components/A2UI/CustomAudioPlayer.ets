/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { media } from '@kit.MediaKit';
import { CustomComponentAttribute } from './CustomComponent';
import { CustomComponentDefinition } from './CustomComponentFactory';
import {
  isSupportedMediaUrl,
  parseCustomProps,
  resolveCustomComponentWarningPathPrefix,
  resolveDynamicStringOptionValue,
  withAttributeSchemaWarningContext,
  resolveMargin,
  Margin,
  buildCustomPropsSignature
} from './CustomComponentUtils';
import { SchemaErrorCode, SchemaErrorInfoManager } from '../../schema/SchemaErrorInfoManager';
import { SchemaResourceLoader } from '../../schema/SchemaResourceLoader';
import { SchemaLocalPropertyHelper } from '../../schema/SchemaLocalPropertyHelper';
import { SchemaProvider } from '../../types/types';
import { CatalogItem } from '../../base/CatalogItem';
import { ThemeMode } from '../../base/NativeEngineBridge';

const AUDIO_PLAYER_TYPE: string = 'AudioPlayer';
const DEFAULT_AUDIO_URL: string = '';
const PLAY_ICON_RESOURCE: string = 'sys.symbol.play_fill';
const PAUSE_ICON_RESOURCE: string = 'sys.symbol.pause';
const AUDIO_PLAYER_ICON_SIZE_RESOURCE: Resource = $r('app.float.custom_audio_player_icon_size');
const LOADING_COLOR_RESOURCE: Resource = $r('app.color.custom_audio_player_loading');
const LOADING_DARK_COLOR_RESOURCE: Resource = $r('app.color.custom_audio_player_loading_dark');
const AUDIO_PLAYER_ICON_LIGHT_COLOR_RESOURCE: Resource = $r('app.color.custom_audio_player_icon_light');
const AUDIO_PLAYER_ICON_DARK_COLOR_RESOURCE: Resource = $r('app.color.custom_audio_player_icon_dark');
const PLAYER_STATE_TIMEOUT_MS: number = 5000;
const MIN_LOADING_VISIBLE_MS: number = 400;
const LOADING_FRAME_DELAY_MS: number = 16;
const AUDIO_SUPPORTED_EXTENSIONS: string[] = [
  '.mp3',
  '.m4a',
  '.aac',
  '.wav',
  '.ogg',
  '.oga',
  '.flac',
  '.amr',
  '.opus'
];

function recordAudioPlayerWarning(code: SchemaErrorCode, message: string, path: string): void {
  SchemaErrorInfoManager.getInstance().recordSchemaWarning(code, message, path, 'component', 'AudioPlayer');
}

function resolveAudioPlayerWarningPath(
  attribute: CustomComponentAttribute | undefined,
  key: string
): string {
  return `${resolveCustomComponentWarningPathPrefix(attribute)}.${key}`;
}

function resolveAudioPlayerDynamicStringProperty(
  source: Record<string, Object>,
  key: string,
  attribute: CustomComponentAttribute | undefined,
  required: boolean = false,
  allowEmpty: boolean = false,
  propertyPath?: string
): string | undefined {
  const rawValue: Object | undefined = source[key];
  const warningPath: string = propertyPath ?? resolveAudioPlayerWarningPath(attribute, key);
  if (rawValue === undefined) {
    if (required) {
      recordAudioPlayerWarning(SchemaErrorCode.REQUIRED_MISS, `audioPlayer Property ${key} is required`, warningPath);
    }
    return undefined;
  }

  if (typeof rawValue === 'string') {
    if (allowEmpty || rawValue.length > 0) {
      return rawValue;
    }
    recordAudioPlayerWarning(SchemaErrorCode.INVALID_VALUE, `audioPlayer Property ${key} expects non-empty string`, warningPath);
    return undefined;
  }

  if (typeof rawValue === 'object' && rawValue !== null && !Array.isArray(rawValue)) {
    let rawRecord: Record<string, Object> = rawValue as Record<string, Object>;
    if (typeof rawRecord['path'] === 'string' || typeof rawRecord['call'] === 'string') {
      return resolveDynamicStringOptionValue(rawValue, attribute);
    }

    if (typeof rawRecord['value'] === 'string') {
      let objectValue: string = rawRecord['value'] as string;
      if (allowEmpty || objectValue.length > 0) {
        recordAudioPlayerWarning(
          SchemaErrorCode.TYPE_MISMATCH,
          `audioPlayer Property ${key} expects DynamicString, object.value has been coerced`,
          warningPath
        );
        return objectValue;
      }
      recordAudioPlayerWarning(SchemaErrorCode.INVALID_VALUE, `audioPlayer Property ${key} expects non-empty string`, warningPath);
      return undefined;
    }
  }

  recordAudioPlayerWarning(
    SchemaErrorCode.TYPE_MISMATCH,
    `audioPlayer Property ${key} expects DynamicString compatible value, fallback to default value`,
    warningPath
  );
  return undefined;
}

export interface AudioPlayerOptions {
  url: string;
  description: string;
  renderContent: boolean;
}

function createDefaultAudioPlayerOptions(): AudioPlayerOptions {
  return {
    url: DEFAULT_AUDIO_URL,
    description: '',
    renderContent: true
  };
}

export function resolveAudioPlayerLoadingColorByThemeMode(colorMode: ThemeMode | undefined): Resource {
  return colorMode === ThemeMode.DARK ? LOADING_DARK_COLOR_RESOURCE : LOADING_COLOR_RESOURCE;
}

export function resolveAudioPlayerIconColorByThemeMode(colorMode: ThemeMode | undefined): Resource {
  return colorMode === ThemeMode.DARK ? AUDIO_PLAYER_ICON_DARK_COLOR_RESOURCE : AUDIO_PLAYER_ICON_LIGHT_COLOR_RESOURCE;
}

export function isSupportedAudioUrl(url: string): boolean {
  return isSupportedMediaUrl(url, AUDIO_SUPPORTED_EXTENSIONS, 'data:audio/');
}

export function normalizeAudioPlayerOptionsForSchemaWarning(
  sourceOptions: AudioPlayerOptions,
  urlWarningPath: string
): AudioPlayerOptions {
  let options: AudioPlayerOptions = {
    url: sourceOptions.url,
    description: sourceOptions.description,
    renderContent: sourceOptions.renderContent
  };

  if (options.url.length > 0 && !isSupportedAudioUrl(options.url)) {
    recordAudioPlayerWarning(
      SchemaErrorCode.INVALID_VALUE,
      'Property url expects a supported audio resource format, drop current audioPlayer component',
      urlWarningPath
    );
    options.url = DEFAULT_AUDIO_URL;
    options.renderContent = false;
  }
  return options;
}

export function resolveAudioPlayerOptionsForSchemaWarning(
  attribute: CustomComponentAttribute | undefined
): AudioPlayerOptions {
  return withAttributeSchemaWarningContext(attribute, (): AudioPlayerOptions => {
    let parsed = parseCustomProps(attribute, 'CustomAudioPlayer');
    const warningPathPrefix: string = resolveCustomComponentWarningPathPrefix(attribute);
    const urlWarningPath: string = resolveAudioPlayerWarningPath(attribute, 'url');
    if (parsed === undefined) {
      if (attribute?.customProps === undefined || attribute?.customProps === null) {
        recordAudioPlayerWarning(SchemaErrorCode.REQUIRED_MISS, 'Property url is required', urlWarningPath);
      }
      return createDefaultAudioPlayerOptions();
    }

    SchemaLocalPropertyHelper.recordUnknownFields(parsed, ['url', 'description'], 'AudioPlayer', warningPathPrefix);

    let options: AudioPlayerOptions = createDefaultAudioPlayerOptions();
    let urlValue: string | undefined =
      resolveAudioPlayerDynamicStringProperty(parsed, 'url', attribute, true, false, urlWarningPath);
    if (urlValue !== undefined && urlValue.length > 0) {
      options.url = urlValue;
      options = normalizeAudioPlayerOptionsForSchemaWarning(options, urlWarningPath);
      if (!options.renderContent) {
        return options;
      }
    }

    let descriptionValue: string | undefined =
      resolveAudioPlayerDynamicStringProperty(
        parsed,
        'description',
        attribute,
        false,
        true,
        resolveAudioPlayerWarningPath(attribute, 'description')
      );
    if (descriptionValue !== undefined) {
      options.description = descriptionValue;
    }

    return options;
  });
}

interface AudioPlaybackController {
  pause: () => Promise<void>;
}

class AudioPlaybackCoordinator {
  private static nextPlayerId: number = 0;
  private static activePlayerId: string = '';
  private static controllers: Map<string, AudioPlaybackController> = new Map<string, AudioPlaybackController>();

  static CreatePlayerId(): string {
    AudioPlaybackCoordinator.nextPlayerId += 1;
    return `audio-player-${AudioPlaybackCoordinator.nextPlayerId}`;
  }

  static Register(playerId: string, controller: AudioPlaybackController): void {
    AudioPlaybackCoordinator.controllers.set(playerId, controller);
  }

  static Unregister(playerId: string): void {
    AudioPlaybackCoordinator.controllers.delete(playerId);
    AudioPlaybackCoordinator.ClearActive(playerId);
  }

  static async RequestActive(playerId: string): Promise<void> {
    if (AudioPlaybackCoordinator.activePlayerId.length > 0 && AudioPlaybackCoordinator.activePlayerId !== playerId) {
      let controller = AudioPlaybackCoordinator.controllers.get(AudioPlaybackCoordinator.activePlayerId);
      if (controller !== undefined) {
        await controller.pause();
      }
    }
    AudioPlaybackCoordinator.activePlayerId = playerId;
  }

  static ClearActive(playerId: string): void {
    if (AudioPlaybackCoordinator.activePlayerId === playerId) {
      AudioPlaybackCoordinator.activePlayerId = '';
    }
  }
}

@Component
export struct CustomAudioPlayer {
  @Watch('handleAttributeChange')
  @Prop attribute: CustomComponentAttribute | undefined = undefined;
  @State avPlayer: media.AVPlayer | null = null;
  @State isPlaying: boolean = false;
  @State isLoading: boolean = false;
  @State isPrepared: boolean = false;
  private currentUrl: string = DEFAULT_AUDIO_URL;
  private prepareSequence: number = 0;
  private readonly playerId: string = AudioPlaybackCoordinator.CreatePlayerId();
  private resolvedOptions: AudioPlayerOptions = createDefaultAudioPlayerOptions();
  private hasResolvedOptions: boolean = false;
  private lastResolvedSignature: string = '';

  aboutToAppear(): void {
    console.log('ArkTS CustomAudioPlayer start build');
    AudioPlaybackCoordinator.Register(this.playerId, { pause: this.pauseForExclusivePlayback });
    this.ensureResolvedOptionsSynchronized();
    this.prepareForCurrentUrl();
  }

  aboutToDisappear(): void {
    this.prepareSequence += 1;
    this.isLoading = false;
    AudioPlaybackCoordinator.Unregister(this.playerId);
    this.releasePlayer();
  }

  build() {
    if (this.shouldRenderAudioPlayer()) {
      this.AudioPlayerRoot();
    }
  }

  @Builder
  private AudioPlayerRoot() {
    Column({ space: 8 }) {
      Stack() {
        if (this.shouldShowLoading()) {
          LoadingProgress()
            .width(this.resolveIconSize())
            .height(this.resolveIconSize())
            .color(this.resolveLoadingColor());
        } else {
          SymbolGlyph(this.resolveActionResource())
            .fontSize(this.resolveIconSize())
            .fontColor([this.resolveActionIconColor()]);
        }
      }
      .alignContent(Alignment.Center)
      .opacity(this.resolveButtonOpacity())
      .accessibilityGroup(true)
      .accessibilityText(this.resolveAccessibilityLabel())
      .accessibilityDescription(this.resolveAccessibilityDescription())
      .onClick(() => {
        if (this.canTogglePlayback()) {
          this.togglePlay();
        }
      });
    }
    .alignItems(HorizontalAlign.Center)
    .layoutWeight(this.resolveWeight())
    .margin(this.resolveMargin())
    .id(this.attribute?.id ?? '')
  }

  private resolveWeight(): number {
    let weight = this.attribute?.properties?.weight;
    if (weight !== undefined && weight > 0) {
      return weight;
    }
    return 0;
  }

  private resolveMargin(): Margin | undefined {
    return resolveMargin(this.attribute);
  }

  private handleAttributeChange(): void {
    this.ensureResolvedOptionsSynchronized();
    this.prepareForCurrentUrl();
  }

  private ensureResolvedOptionsSynchronized(): void {
    let currentSignature: string = buildCustomPropsSignature(this.attribute);
    if (this.hasResolvedOptions && this.lastResolvedSignature === currentSignature) {
      return;
    }
    this.synchronizeResolvedOptions();
  }

  private synchronizeResolvedOptions(): void {
    this.resolvedOptions = this.resolveOptions();
    this.hasResolvedOptions = true;
    this.lastResolvedSignature = buildCustomPropsSignature(this.attribute);
  }

  private pauseForExclusivePlayback = async (): Promise<void> => {
    if (this.avPlayer === null) {
      this.isPlaying = false;
      return;
    }

    if (this.avPlayer.state !== 'playing') {
      this.isPlaying = false;
      return;
    }

    try {
      await this.avPlayer.pause();
      this.isPlaying = false;
    } catch (error) {
      console.error(`CustomAudioPlayer pauseForExclusivePlayback failed: ${JSON.stringify(error)}`);
    }
  };

  private async prepareForCurrentUrl(): Promise<void> {
    let url = this.resolveUrl();
    let sequence = ++this.prepareSequence;
    let loadingStartMs = Date.now();

    if (url.length === 0) {
      this.isLoading = false;
      this.releasePlayer();
      return;
    }

    if (this.avPlayer !== null && this.currentUrl === url) {
      this.isLoading = false;
      this.isPrepared = await this.ensurePlayerPlayable(this.avPlayer);
      return;
    }

    this.isLoading = true;
    this.isPrepared = false;
    this.isPlaying = false;
    await this.waitForLoadingFrame(sequence);
    if (sequence !== this.prepareSequence) {
      return;
    }
    await this.releasePlayer();
    if (sequence !== this.prepareSequence) {
      return;
    }

    let prepared = await this.initPlayer(url, sequence);
    if (sequence !== this.prepareSequence) {
      return;
    }

    await this.waitForMinimumLoading(sequence, loadingStartMs);
    if (sequence !== this.prepareSequence) {
      return;
    }
    this.isPrepared = prepared;
    this.isLoading = false;
  }

  private async togglePlay(): Promise<void> {
    if (!await this.ensurePlayerReady()) {
      return;
    }

    if (this.avPlayer === null) {
      return;
    }

    try {
      if (this.avPlayer.state === 'playing') {
        await this.avPlayer.pause();
        this.isPlaying = false;
        return;
      }

      if (!await this.ensurePlayerPlayable(this.avPlayer)) {
        return;
      }

      if (this.avPlayer.state === 'prepared' || this.avPlayer.state === 'paused' ||
        this.avPlayer.state === 'completed') {
        await AudioPlaybackCoordinator.RequestActive(this.playerId);
        await this.avPlayer.play();
        this.isPlaying = true;
        return;
      }

      console.warn(`CustomAudioPlayer togglePlay skipped, unsupported state: ${this.avPlayer.state}`);
    } catch (error) {
      console.error(`CustomAudioPlayer togglePlay failed: ${JSON.stringify(error)}`);
      AudioPlaybackCoordinator.ClearActive(this.playerId);
      this.isPlaying = false;
    }
  }

  private async ensurePlayerReady(): Promise<boolean> {
    let url = this.resolveUrl();
    if (url.length === 0) {
      return false;
    }

    if (this.isLoading) {
      return false;
    }

    if (this.avPlayer !== null && this.currentUrl === url) {
      let playable = await this.ensurePlayerPlayable(this.avPlayer);
      this.isPrepared = playable;
      return playable;
    }

    await this.prepareForCurrentUrl();
    return this.isPrepared;
  }

  private async initPlayer(url: string, sequence: number): Promise<boolean> {
    let player: media.AVPlayer | null = null;

    try {
      player = await media.createAVPlayer();
      if (sequence !== this.prepareSequence) {
        await this.disposePlayer(player);
        return false;
      }

      this.avPlayer = player;
      this.currentUrl = url;
      this.attachPlayerListeners(player);
      player.url = url;

      if (player.state !== 'initialized') {
        let initialized = await this.waitForAnyPlayerState(player, ['initialized']);
        if (!initialized) {
          throw new Error(`AVPlayer did not enter initialized state, current state: ${player.state}`);
        }
      }

      await player.prepare();
      return await this.waitForAnyPlayerState(player, ['prepared', 'paused', 'completed', 'playing']);
    } catch (error) {
      console.error(`CustomAudioPlayer initPlayer failed: ${JSON.stringify(error)}`);
      if (this.avPlayer === player) {
        this.avPlayer = null;
        this.currentUrl = DEFAULT_AUDIO_URL;
      }
      this.isPlaying = false;
      this.isPrepared = false;
      await this.disposePlayer(player);
      this.dispatchRuntimeAudioWarning('AudioPlayer url failed to initialize as an audio resource');
      return false;
    }
  }

  private attachPlayerListeners(player: media.AVPlayer): void {
    player.on('stateChange', (state: media.AVPlayerState) => {
      if (this.avPlayer !== player) {
        return;
      }

      console.log(`CustomAudioPlayer stateChange: ${state}`);
      if (state === 'prepared' || state === 'paused' || state === 'completed' || state === 'playing') {
        this.isLoading = false;
        this.isPrepared = true;
      }

      if (state === 'prepared') {
        this.isPlaying = false;
      } else if (state === 'playing') {
        this.isPlaying = true;
      } else if (state === 'paused' || state === 'completed') {
        this.isPlaying = false;
        AudioPlaybackCoordinator.ClearActive(this.playerId);
      } else if (state === 'released' || state === 'stopped' || state === 'error') {
        this.isPlaying = false;
        this.isPrepared = false;
        this.isLoading = false;
        AudioPlaybackCoordinator.ClearActive(this.playerId);
      }
    });

    player.on('error', (error: Error) => {
      if (this.avPlayer !== player) {
        return;
      }
      console.error(`CustomAudioPlayer player error: ${JSON.stringify(error)}`);
      this.dispatchRuntimeAudioWarning('AudioPlayer url failed during audio playback');
      this.isPlaying = false;
      this.isPrepared = false;
      this.isLoading = false;
      AudioPlaybackCoordinator.ClearActive(this.playerId);
    });
  }

  private async ensurePlayerPlayable(player: media.AVPlayer): Promise<boolean> {
    if (player.state === 'prepared' || player.state === 'paused' || player.state === 'completed' ||
      player.state === 'playing') {
      return true;
    }

    if (player.state === 'initialized') {
      try {
        this.isLoading = true;
        await player.prepare();
      } catch (error) {
        console.error(`CustomAudioPlayer ensurePlayerPlayable prepare failed: ${JSON.stringify(error)}`);
        this.dispatchRuntimeAudioWarning('AudioPlayer url failed to prepare as an audio resource');
        this.isLoading = false;
        return false;
      }
    }

    let playable = await this.waitForAnyPlayerState(player, ['prepared', 'paused', 'completed', 'playing']);
    if (!playable) {
      this.isLoading = false;
    }
    return playable;
  }

  private async waitForAnyPlayerState(player: media.AVPlayer, targetStates: media.AVPlayerState[]): Promise<boolean> {
    for (const state of targetStates) {
      if (player.state === state) {
        return true;
      }
    }

    if (player.state === 'error' || player.state === 'released' || player.state === 'stopped') {
      return false;
    }

    return await new Promise<boolean>((resolve: (value: boolean) => void) => {
      let settled: boolean = false;
      let timerId: number = -1;

      let cleanup = () => {
        if (timerId >= 0) {
          clearTimeout(timerId);
        }
        player.off('stateChange', stateChangeCallback);
        player.off('error', errorCallback);
      };

      let finish = (result: boolean) => {
        if (settled) {
          return;
        }
        settled = true;
        cleanup();
        resolve(result);
      };

      let stateChangeCallback = (state: media.AVPlayerState) => {
        for (const targetState of targetStates) {
          if (state === targetState) {
            finish(true);
            return;
          }
        }

        if (state === 'error' || state === 'released' || state === 'stopped') {
          finish(false);
        }
      };

      let errorCallback = (_error: Error) => {
        finish(false);
      };

      player.on('stateChange', stateChangeCallback);
      player.on('error', errorCallback);
      timerId = setTimeout(() => {
        console.warn(`CustomAudioPlayer waitForPlayerState timeout, target=${JSON.stringify(targetStates)}, current=${player.state}`);
        finish(false);
      }, PLAYER_STATE_TIMEOUT_MS);
    });
  }

  private async waitForLoadingFrame(sequence: number): Promise<void> {
    await this.delay(LOADING_FRAME_DELAY_MS);
    if (sequence !== this.prepareSequence) {
      return;
    }
  }

  private async waitForMinimumLoading(sequence: number, loadingStartMs: number): Promise<void> {
    let remainMs = MIN_LOADING_VISIBLE_MS - (Date.now() - loadingStartMs);
    if (remainMs <= 0) {
      return;
    }
    await this.delay(remainMs);
    if (sequence !== this.prepareSequence) {
      return;
    }
  }

  private async delay(durationMs: number): Promise<void> {
    await new Promise<void>((resolve: () => void) => {
      setTimeout(() => {
        resolve();
      }, durationMs);
    });
  }

  private async releasePlayer(): Promise<void> {
    let player = this.avPlayer;
    AudioPlaybackCoordinator.ClearActive(this.playerId);
    this.avPlayer = null;
    this.currentUrl = DEFAULT_AUDIO_URL;
    this.isPlaying = false;
    this.isPrepared = false;
    await this.disposePlayer(player);
  }

  private async disposePlayer(player: media.AVPlayer | null): Promise<void> {
    if (player === null) {
      return;
    }

    try {
      await player.release();
    } catch (error) {
      console.error(`CustomAudioPlayer releasePlayer failed: ${JSON.stringify(error)}`);
    }
  }

  private resolveUrl(): string {
    this.ensureResolvedOptionsSynchronized();
    return this.resolvedOptions.url;
  }

  private shouldRenderAudioPlayer(): boolean {
    this.ensureResolvedOptionsSynchronized();
    return this.resolvedOptions.renderContent;
  }

  private hasAudioUrl(): boolean {
    return this.resolveUrl().length > 0;
  }

  private shouldShowLoading(): boolean {
    return this.hasAudioUrl() && this.isLoading;
  }

  private canTogglePlayback(): boolean {
    return this.hasAudioUrl() && !this.isLoading && this.isPrepared;
  }

  private resolveAccessibilityLabel(): string {
    return this.attribute?.properties?.accessibilityLabel ?? '';
  }

  private resolveAccessibilityDescription(): string {
    return this.attribute?.properties?.accessibilityDescription ?? '';
  }

  private resolveActionResource(): Resource {
    return $r(this.isPlaying ? PAUSE_ICON_RESOURCE : PLAY_ICON_RESOURCE);
  }

  private resolveIconSize(): Resource {
    return AUDIO_PLAYER_ICON_SIZE_RESOURCE;
  }

  private resolveLoadingColor(): Resource {
    return resolveAudioPlayerLoadingColorByThemeMode(this.attribute?.componentTheme?.colorMode);
  }

  private resolveActionIconColor(): Resource {
    return resolveAudioPlayerIconColorByThemeMode(this.attribute?.componentTheme?.colorMode);
  }

  private resolveButtonOpacity(): number {
    if (!this.hasAudioUrl()) {
      return 0.5;
    }

    return this.canTogglePlayback() ? 1 : 0.6;
  }

  private resolveOptions(): AudioPlayerOptions {
    return resolveAudioPlayerOptionsForSchemaWarning(this.attribute);
  }

  private dispatchRuntimeAudioWarning(message: string): void {
    const schemaErrorInfoManager = SchemaErrorInfoManager.getInstance();
    const urlWarningPath: string = resolveAudioPlayerWarningPath(this.attribute, 'url');
    schemaErrorInfoManager.withSchemaWarningContext<void>(
      this.attribute?.renderId,
      this.attribute?.surfaceId,
      () => {
        const captureResult = schemaErrorInfoManager.captureWarnings<void>(() => {
          recordAudioPlayerWarning(SchemaErrorCode.INVALID_VALUE, message, urlWarningPath);
        });
        schemaErrorInfoManager.dispatchCapturedWarnings(this.attribute?.surfaceId, captureResult.warnings);
      }
    );
  }
}

@Builder
export function customAudioPlayerBuilder(attribute: CustomComponentAttribute): void {
  CustomAudioPlayer({ attribute: attribute });
}

export function createAudioPlayerDefinition(): CustomComponentDefinition {
  const schemaProvider: SchemaProvider = (version: string) => {
    return SchemaResourceLoader.loadA2UISchema(version, 'components/AudioPlayer.json');
  };
  return {
    type: AUDIO_PLAYER_TYPE,
    builder: wrapBuilder(customAudioPlayerBuilder),
    schemaProvider: schemaProvider
  };
}

export function asCatalogItem(): CatalogItem {
  const definition: CustomComponentDefinition = createAudioPlayerDefinition();
  return CatalogItem.forComponent(
    definition.type,
    definition.schemaProvider ?? ((_version: string): string => ''),
    definition.builder
  ).markInnerNative(false);
}
