using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Threading.Tasks;

namespace task2;

public class DogApiResponse
{
    [JsonPropertyName("message")]
    public required string Message { get; set; }

    [JsonPropertyName("status")]
    public required string Status { get; set; }
}

class Program
{
    private static readonly HttpClient HttpClient = new();
    private const string ApiUrl = "https://dog.ceo/api/breeds/image/random";
    private const int ImageCount = 10;
    private const string SequentialFolder = "../../../sequential";
    private const string ParallelFolder = "../../../parallel";
    private const int MaxDownloadTimeMs = 3000;

    static async Task Main()
    {
        Directory.CreateDirectory(SequentialFolder);
        Console.WriteLine($"=== Последовательная загрузка ({ImageCount} изображений) ===");
        Console.WriteLine($"Максимальное время на картинку: {MaxDownloadTimeMs} мс\n");

        var sequentialTimer = Stopwatch.StartNew();
        await DownloadImagesSequentiallyAsync(SequentialFolder);
        sequentialTimer.Stop();

        Console.WriteLine($"\n Последовательная загрузка завершена за {sequentialTimer.ElapsedMilliseconds} мс\n");

        Directory.CreateDirectory(ParallelFolder);
        Console.WriteLine($"=== Параллельная загрузка ({ImageCount} изображений) ===");
        Console.WriteLine($"Максимальное время на картинку: {MaxDownloadTimeMs} мс\n");

        var parallelTimer = Stopwatch.StartNew();
        await DownloadImagesInParallelAsync();
        parallelTimer.Stop();

        Console.WriteLine($"\n Параллельная загрузка завершена за {parallelTimer.ElapsedMilliseconds} мс\n");
    }
    static async Task DownloadImagesSequentiallyAsync(string folder)
    {
        for (int i = 0; i < ImageCount; i++)
        {
            int imageIndex = i + 1;
            using var cts = new CancellationTokenSource(MaxDownloadTimeMs);
            CancellationToken token = cts.Token;

            try
            {
                string imageUrl = await GetRandomDogImageUrlAsync(token);
                Console.WriteLine($"[SEQUENTIAL][{imageIndex}] URL: {Path.GetFileName(imageUrl)}");

                string filePath = Path.Combine(folder, $"dog_{imageIndex}.jpg");
                await DownloadImageAsync(imageUrl, filePath, token);

                Console.WriteLine($"[SEQUENTIAL][{imageIndex}] Успешно");
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                Console.WriteLine($"[SEQUENTIAL][{imageIndex}] Таймаут ({MaxDownloadTimeMs} мс)");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[SEQUENTIAL][{imageIndex}] {ex.Message}");
            }
        }
    }

    static async Task DownloadImagesInParallelAsync()
    {
        var tasks = new List<Task>();

        for (int i = 0; i < ImageCount; i++)
        {
            int imageIndex = i + 1;
            tasks.Add(Task.Run(async () =>
            {
                using var cts = new CancellationTokenSource(MaxDownloadTimeMs);
                CancellationToken token = cts.Token;

                try
                {
                    string imageUrl = await GetRandomDogImageUrlAsync(token);
                    Console.WriteLine($"[PARALLEL][{imageIndex}] URL: {Path.GetFileName(imageUrl)}");

                    string filePath = Path.Combine(ParallelFolder, $"dog_{imageIndex}.jpg");
                    await DownloadImageAsync(imageUrl, filePath, token);

                    Console.WriteLine($"[PARALLEL][{imageIndex}] Успешно");
                }
                catch (OperationCanceledException) when (token.IsCancellationRequested)
                {
                    Console.WriteLine($"[PARALLEL][{imageIndex}] Таймаут ({MaxDownloadTimeMs} мс)");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"[PARALLEL][{imageIndex}] {ex.Message}");
                }
            }));
        }

        await Task.WhenAll(tasks);
    }

    static async Task<string> GetRandomDogImageUrlAsync(CancellationToken cancellationToken)
    {
        string response = await HttpClient.GetStringAsync(ApiUrl, cancellationToken);
        var dogResponse = JsonSerializer.Deserialize<DogApiResponse>(response);

        if (dogResponse?.Status != "success" || string.IsNullOrEmpty(dogResponse.Message))
        {
            throw new InvalidOperationException($"API Error: {dogResponse?.Status}");
        }

        return dogResponse.Message;
    }

    static async Task DownloadImageAsync(string url, string filePath, CancellationToken cancellationToken)
    {
        using var response = await HttpClient.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
        response.EnsureSuccessStatusCode();

        await using var stream = await response.Content.ReadAsStreamAsync(cancellationToken);
        await using var fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write, FileShare.None, bufferSize: 8192, useAsync: true);

        byte[] buffer = new byte[8192];
        int bytesRead;

        while ((bytesRead = await stream.ReadAsync(buffer, cancellationToken)) > 0)
        {
            await fileStream.WriteAsync(buffer.AsMemory(0, bytesRead), cancellationToken);
            cancellationToken.ThrowIfCancellationRequested();
        }
    }
}