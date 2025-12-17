using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Threading.Tasks;

class Program
{
    static async Task Main(string[] args)
    {
        if (args.Length == 0)
        {
            Console.WriteLine("Ошибка: Не указан путь к файлу.");
            Console.WriteLine("Использование: app.exe <путь_к_файлу>");
            return;
        }

        string filePath = args[0];

        try
        {
            if (!File.Exists(filePath))
            {
                Console.WriteLine($"Ошибка: Файл по пути '{filePath}' не существует.");
                return;
            }

            Console.WriteLine($"Чтение файла: {filePath}");
            string content = await File.ReadAllTextAsync(filePath);

            Console.Write("Введите символы для удаления (без разделителей): ");
            string charsInput = Console.ReadLine()!;
            var charsToRemove = new HashSet<char>(charsInput);

            Console.WriteLine("Обработка текста...");
            string cleanedContent = new([.. content.Where(c => !charsToRemove.Contains(c))]);

            Console.WriteLine("Сохранение изменений в файл...");
            await File.WriteAllTextAsync(filePath, cleanedContent);

            Console.WriteLine("Операция успешно завершена!");
        }
        catch (UnauthorizedAccessException)
        {
            Console.WriteLine("Ошибка: Нет прав для доступа к файлу.");
        }
        catch (IOException ex)
        {
            Console.WriteLine($"Ошибка ввода-вывода: {ex.Message}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Неожиданная ошибка: {ex.Message}");
        }
    }
}